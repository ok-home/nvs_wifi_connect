#include "prv_wifi_connect_private.h"
#include "prv_wifi_connect.h"

#include "freertos/task.h"
#include "freertos/queue.h"

#include "jsmn.h"

static const char *TAG = "prv_http_server";

static int srv_restart = 0;

// simple json parse -> only one parametr name/val
static esp_err_t json_to_str_parm(char *jsonstr, char *nameStr, char *valStr) // распаковать строку json в пару  name/val
{
    int r; // количество токенов
    jsmn_parser p;
    jsmntok_t t[5]; // только 2 пары параметров и obj

    jsmn_init(&p);
    r = jsmn_parse(&p, jsonstr, strlen(jsonstr), t, sizeof(t) / sizeof(t[0]));
    if (r < 2)
    {
        valStr[0] = 0;
        nameStr[0] = 0;
        return ESP_FAIL;
    }
    strncpy(nameStr, jsonstr + t[2].start, t[2].end - t[2].start);
    nameStr[t[2].end - t[2].start] = 0;
    if (r > 3)
    {
        strncpy(valStr, jsonstr + t[4].start, t[4].end - t[4].start);
        valStr[t[4].end - t[4].start] = 0;
    }
    else
        valStr[0] = 0;
    return ESP_OK;
}

static void send_json_string(char *str, httpd_req_t *req)
{
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)str;
    ws_pkt.len = strlen(str);
    httpd_ws_send_frame(req, &ws_pkt);
}

static void send_nvs_data(httpd_req_t *req)
{
    char buf[128] = {0};
    char nvs_data[64] = {0};
    nvs_handle_t nvs_handle;
    size_t required_size = 0;

    nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &nvs_handle);
    required_size = sizeof(nvs_data);
    nvs_get_str(nvs_handle, NVS_STA_AP_DEFAULT_MODE_KEY, nvs_data, &required_size);
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"msg\":\"%s\"}", NVS_STA_AP_DEFAULT_MODE_KEY, nvs_data);
    send_json_string(buf, req);
    required_size = sizeof(nvs_data);
    nvs_get_str(nvs_handle, NVS_AP_ESP_WIFI_SSID_KEY, nvs_data, &required_size);
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"msg\":\"%s\"}", NVS_AP_ESP_WIFI_SSID_KEY, nvs_data);
    send_json_string(buf, req);
    required_size = sizeof(nvs_data);
    nvs_get_str(nvs_handle, NVS_AP_ESP_WIFI_PASS_KEY, nvs_data, &required_size);
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"msg\":\"%s\"}", NVS_AP_ESP_WIFI_PASS_KEY, nvs_data);
    send_json_string(buf, req);
    required_size = sizeof(nvs_data);
    nvs_get_str(nvs_handle, NVS_STA_ESP_WIFI_SSID_KEY, nvs_data, &required_size);
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"msg\":\"%s\"}", NVS_STA_ESP_WIFI_SSID_KEY, nvs_data);
    send_json_string(buf, req);
    required_size = sizeof(nvs_data);
    nvs_get_str(nvs_handle, NVS_STA_ESP_WIFI_PASS_KEY, nvs_data, &required_size);
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"msg\":\"%s\"}", NVS_STA_ESP_WIFI_PASS_KEY, nvs_data);
    send_json_string(buf, req);
    nvs_close(nvs_handle);
}

static void set_nvs_data(char *jsonstr)
{
    char key[16];
    char value[64];
    nvs_handle_t nvs_handle;
    nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &nvs_handle);
    esp_err_t err = json_to_str_parm(jsonstr, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "ERR jsonstr %s", jsonstr);
    }
    else
    {
        if (strncmp(key, NVS_WIFI_RESTART_KEY, strlen(NVS_WIFI_RESTART_KEY)) != 0) // key/value -> wifi data
        {
            if (nvs_set_str(nvs_handle, key, value))
            {
                ESP_LOGE(TAG, "ERR WRITE key %s value %s", key, value);
            }
        }
        else // key/value ->  restart or write
        {
            nvs_commit(nvs_handle);
            if (srv_restart || strncmp(value, NVS_WIFI_RESTART_VALUE_RESTART, strlen(NVS_WIFI_RESTART_VALUE_RESTART)) == 0)
            {
                ESP_LOGI(TAG, "restart key %s value %s", key, value);
                nvs_close(nvs_handle);
                esp_restart();
            }
        }
    }
    nvs_close(nvs_handle);
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
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
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    if (ws_pkt.len)
    {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
    }
    set_nvs_data((char *)ws_pkt.payload);
    free(buf);
    return ret;
}
static esp_err_t get_handler(httpd_req_t *req)
{
    extern const unsigned char prv_wifi_connect_html_start[] asm("_binary_prv_wifi_connect_html_start");
    extern const unsigned char prv_wifi_connect_html_end[] asm("_binary_prv_wifi_connect_html_end");
    const size_t prv_wifi_connect_html_size = (prv_wifi_connect_html_end - prv_wifi_connect_html_start);

    httpd_resp_send_chunk(req, (const char *)prv_wifi_connect_html_start, prv_wifi_connect_html_size);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
static const httpd_uri_t gh = {
    .uri = DEFAULT_URI,
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL};
static const httpd_uri_t ws = {
    .uri = DEFAULT_WS_URI,
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true};
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        if (prv_register_uri_handler(server) == ESP_OK)
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

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

esp_err_t prv_register_uri_handler(httpd_handle_t server)
{
    esp_err_t ret = ESP_OK;
    ret = httpd_register_uri_handler(server, &gh);
    if (ret)
        goto _ret;
    ret = httpd_register_uri_handler(server, &ws);
    if (ret)
        goto _ret;
_ret:
    return ret;
}

void prv_start_http_server(int restart)
{
    static httpd_handle_t server = NULL;
    srv_restart = restart;

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();
}