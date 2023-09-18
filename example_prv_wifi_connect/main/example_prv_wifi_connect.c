#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>

#include "prv_wifi_connect.h"

void app_main(void)
{
    prv_wifi_connect();                     // return with error
    prv_start_http_server(PRV_MODE_STOP_SERVER, NULL); // run server
}
