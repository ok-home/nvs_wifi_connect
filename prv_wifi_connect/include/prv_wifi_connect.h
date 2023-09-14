#pragma once

#define DEFAULT_URI "/"
#define DEFAULT_WS_URI "/ws"

/*
*   @brief  Connect to wifi AP or STA mode
*           ssid & pass - get from nvs
*           AP or STA - get from nvs
*           if nvs fail or not found -> go to default AP mode -> start WS server form
*           if STA connection fail -> go to AP mode and WS server form
*
*/
esp_err_t prv_wifi_connect(void);

void wifi_init_softap(char *ap_ssid, char *ap_pass);
esp_err_t wifi_init_sta(char *sta_ssid, char *sta_pass);

esp_err_t prv_register_uri_handler(httpd_handle_t server);
void prv_start_http_server(int restart)