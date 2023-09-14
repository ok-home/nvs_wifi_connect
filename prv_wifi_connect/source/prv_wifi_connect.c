#include "prv_wifi_connect_private.h"
#include "prv_wifi_connect.h"


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "prv_wifi_connect";

static int short_retry_num = 0;
//static int long_retry_num = 0;


static void event_handler_sta(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (short_retry_num < STA_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            short_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            // check long retry
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        short_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(char *ap_ssid, char *ap_pass)
{
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    strncpy((char *)wifi_config.ap.ssid,ap_ssid,sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password,ap_pass,sizeof(wifi_config.ap.password));

    if (strlen((char*)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             ap_ssid, ap_pass);
}

esp_err_t wifi_init_sta(char *sta_ssid, char *sta_pass )
{
    esp_err_t err = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();

    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler_sta,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler_sta,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char *)wifi_config.sta.ssid,sta_ssid,sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password,sta_pass,sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 sta_ssid, sta_pass);
                 err = ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 sta_ssid, sta_pass);
                 err = ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        err = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    return err;
}
/*
*   @brief  Connect to wifi AP or STA mode
*           ssid & pass - get from nvs
*           AP or STA - get from nvs
*           if nvs fail or not found -> go to default AP mode -> start WS server form
*           if STA connection fail -> go to AP mode and WS server form
*
*/

esp_err_t prv_wifi_connect(void)
{
    int nvs_init = 0;
    nvs_handle_t nvs_handle;
    size_t required_size;
    char  nvs_mode[32]={0};
    char  nvs_ssid[32]={0};
    char  nvs_password[64]={0};

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_init = 1;
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if(nvs_init || nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &nvs_handle) || nvs_get_str(nvs_handle, NVS_STA_AP_DEFAULT_MODE_KEY, nvs_mode, &required_size) ) // nvs erase -> no valid data for connect -> default AP mode
    {
        ESP_LOGE(TAG,"NVS INIT ERR init=%d,handle=%lx,mode=%s",nvs_init,nvs_handle,nvs_mode);
        err = ESP_FAIL;
    }
    else 
    {
        if(strncmp(NVS_WIFI_MODE_STA,nvs_mode,strlen(NVS_WIFI_MODE_STA)) == 0) // sta
        {
            if((nvs_get_str(nvs_handle, NVS_STA_ESP_WIFI_SSID_KEY, nvs_ssid, &required_size) || nvs_get_str(nvs_handle, NVS_STA_ESP_WIFI_PASS_KEY, nvs_password, &required_size)) == ESP_OK)
            {
            err = wifi_init_sta(nvs_ssid,nvs_password); // ssid & pass OK
            if(err) {ESP_LOGE(TAG,"STA ERR ssid=%s pass=%s",nvs_ssid,nvs_password);}
            }
        }
        else // ap
        {
            err = nvs_get_str(nvs_handle, NVS_AP_ESP_WIFI_SSID_KEY , nvs_ssid, &required_size) || nvs_get_str(nvs_handle, NVS_AP_ESP_WIFI_PASS_KEY , nvs_password, &required_size);
            if( err == ESP_OK)
            {
                wifi_init_softap(nvs_ssid,nvs_password); // ssid & pass OK
            }
            else {ESP_LOGE(TAG,"AP ERR ssid=%s pass=%s",nvs_ssid,nvs_password);}
        }
    }
    nvs_close(nvs_handle);
    if ( err )
    {
        wifi_init_softap(DEFAULT_AP_ESP_WIFI_SSID,DEFAULT_AP_ESP_WIFI_PASS);
        ESP_LOGE(TAG,"ERR start default AP");
    }
    // start default prv server
    //prv_start_http_server(0);
    return err;
}

