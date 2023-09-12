/* WebSocket Echo Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include <esp_http_server.h>

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "prv_http_server";

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

void send_json_string(uint8_t *str, httpd_req_t *req)
{
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = str;
    ws.pkt.len = strlen(str);
    httpd_ws_send_frame(req, &ws_pkt);
}

void send_nvs_data(httpd_req_t *req)
{
    uint8_t buf[128] = {0};
    uint8_t nvs_data[64] = {0};
    nvs_handle_t my_handle;
    int required_size = 0;
    
    nvs_open("storage", NVS_READWRITE, &my_handle);
    nvs_get_str(my_handle, "nvsApStaMode", nvs_data, &required_size);
    snprintf(buf,sizeof(buf),"{\"name\":\"%s\",\"msg\":\"%s\"}","nvsApStaMode",nvs_data);
    send_json_string(buf,req);    
    nvs_get_str(my_handle, "nvsApSsid", nvs_data, &required_size);
    snprintf(buf,sizeof(buf),"{\"name\":\"%s\",\"msg\":\"%s\"}","nvsApSsid",nvs_data);
    send_json_string(buf,req);    
    nvs_get_str(my_handle, "nvsApPass", nvs_data, &required_size);
    snprintf(buf,sizeof(buf),"{\"name\":\"%s\",\"msg\":\"%s\"}","nvsApPass",nvs_data);
    send_json_string(buf,req);    
    nvs_get_str(my_handle, "nvsStaSsid", nvs_data, &required_size);
    snprintf(buf,sizeof(buf),"{\"name\":\"%s\",\"msg\":\"%s\"}","nvsStaSsid",nvs_data);
    send_json_string(buf,req);    
    nvs_get_str(my_handle, "nvsStaPass", nvs_data, &required_size);
    snprintf(buf,sizeof(buf),"{\"name\":\"%s\",\"msg\":\"%s\"}","nvsStaPass",nvs_data);
    send_json_string(buf,req);
    nvs_close(my_handle);
}

void set_nvs_data(char *jsonstr)
{
    char key[16];
    char value[64];
    ESP_LOGI(TAG,"jsonstr=%s",jsonstr);
    int res = sscanf(jsonstr,"{\"name\":\"%s\",\"msg\":\"%s\"}",key,value);
    if(res==2)
    {
        ESP_LOGI(TAG,"key=%s value=%s",key,value);
    }
}

static esp_err_t echo_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        send_nvs_data(req);
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);

    set_nvs_data(ws_pkt.payload);

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}

static esp_err_t get_handler(httpd_req_t *req)
{
    extern const unsigned char prw_wifi_connect_html_start[] asm("_binary_prw_wifi_connect_html_start");
    extern const unsigned char prw_wifi_connect_html_end[] asm("_binary_prw_wifi_connect_html_end");
    const size_t prw_wifi_connect_html_size = (prw_wifi_connect_html_end - prw_wifi_connect_html_start);

    httpd_resp_send_chunk(req, (const char *)prw_wifi_connect_html_start, prw_wifi_connect_html_size);
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}
static const httpd_uri_t gh = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL};
static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        httpd_register_uri_handler(server, &gh);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}


void prv_http_server(void)
{
    static httpd_handle_t server = NULL;


    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();
}