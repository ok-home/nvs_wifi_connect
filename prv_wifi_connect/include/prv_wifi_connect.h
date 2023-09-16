#pragma once

#define DEFAULT_URI "/wifi"
#define DEFAULT_WS_URI "/wifi/ws"

esp_err_t prv_wifi_connect(void);
void prv_wifi_init_softap(char *ap_ssid, char *ap_pass);
esp_err_t prv_wifi_init_sta(char *sta_ssid, char *sta_pass);

esp_err_t prv_register_uri_handler(httpd_handle_t server);
void prv_start_http_server(int restart);