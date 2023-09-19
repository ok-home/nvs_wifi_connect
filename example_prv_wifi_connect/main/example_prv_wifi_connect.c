//#include <esp_log.h>

#include "prv_wifi_connect.h"

void example_echo_ws_server(void);
esp_err_t example_register_uri_handler(httpd_handle_t server);

void app_main(void)
{
    prv_wifi_connect();                     // return with error ?
    prv_start_http_server(PRV_MODE_STAY_ACTIVE,example_register_uri_handler); // run server
    //example_echo_ws_server();

}
