#pragma once

#include <esp_system.h>
#include <esp_http_server.h>

#define DEFAULT_URI "/wifi"
#define DEFAULT_WS_URI "/wifi/ws"

#define DEFAULT_AP_ESP_WIFI_SSID "Provision_Wifi_AP"
#define DEFAULT_AP_ESP_WIFI_PASS ""


enum {
    PRV_MODE_STAY_ACTIVE, // -> no operation
    PRV_MODE_STOP_SERVER, // -> stop httpd
    PRV_MODE_RESTART_ESP32 // -> full restart esp32 regardless of value NVS_WIFI_RESTART_VALUE_RESTART
};

#ifdef __cplusplus
extern "C"
{
#endif

/*
*   @brief  connect to wifi
*           connect mode ( ap/sta , ssid/pass ) read from NVS
*   @return
*           ESP_OK      -> mode read from NVS & connect to wifi OK
*           ESP_FAIL    -> can`t connect to wifi with NVS data, create AP  with DEFAULT_AP_ESP_WIFI_SSID/DEFAULT_AP_ESP_WIFI_PASS
*/
esp_err_t prv_wifi_connect(void);
/*
*   @brief  create softAP with ap_ssid/ap_pass ip=192.168.4.1
*   @param  char *ap_ssid -> softAP ssid
*   @param  char *ap_pass -> softAP pass -> NULL -> authmode = WIFI_AUTH_OPEN
*/
void prv_wifi_init_softap(char *ap_ssid, char *ap_pass);
/*
*   @brief  connect to WIFI with sta_ssid/sta_pass 
*   @param  char *sta_ssid -> WIFI ssid
*   @param  char *sta_pass -> WIFI pass
*   @return
*           ESP_OK      -> connect to wifi OK
*           ESP_FAIL    -> can`t connect to wifi with ssid/pass
*/
esp_err_t prv_wifi_init_sta(char *sta_ssid, char *sta_pass);
/*
*   @brief  register provision handlers ( web page & ws handlers) on existing  httpd server with ws support
*           uri page -> DEFAULT_URI
*   @param  httpd_handle_t server -> existing server handle
*   @return
*           ESP_OK      -> register OK
*           ESP_FAIL    -> register FAIL
*/
esp_err_t prv_register_uri_handler(httpd_handle_t server);

/*
*   @brief handle for connecting to the prv_http_server server
*/
typedef esp_err_t (*prv_wifi_connect_register_uri_handler)(httpd_handle_t server);

/*
*   @brief  start provision httpd server, uri web page read existing nvs wifi data  & write new nvs wifi data ( ap/sta mode, wifi ssid/pass )
*           uri page -> DEFAULT_URI
*   @param int restart_mode
*           PRV_MODE_STAY_ACTIVE   -> write nvs wifi data and stay active connection
*           PRV_MODE_STOP_SERVER   -> stop httpd after write new nvs wifi data
*           PRV_MODE_RESTART_ESP32 -> full restart esp32 regardless of value NVS_WIFI_RESTART_VALUE_RESTART
*   @param  prv_wifi_connect_register_uri_handler register_uri_handle
*           connect other uri handlers with started httpd server
*   @return  
*           httpd_handle_t server -> server handle on OK start
*           NULL                  -> server start FAIL
*/
httpd_handle_t prv_start_http_server(int restart_mode , prv_wifi_connect_register_uri_handler register_uri_handler);

#ifdef __cplusplus
}
#endif