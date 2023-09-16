#pragma once

#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_http_server.h"

#define DEFAULT_AP_ESP_WIFI_SSID "Provision_Wifi_AP"
#define DEFAULT_AP_ESP_WIFI_PASS ""
#define AP_MAX_STA_CONN 4

#define STA_ESP_MAXIMUM_RETRY 5
#define STA_ESP_MAXIMUM_LONG_RETRY 5

#define NVS_AP_ESP_WIFI_SSID_KEY "nvsApSsid"
#define NVS_AP_ESP_WIFI_PASS_KEY "nvsApPass"
#define NVS_STA_ESP_WIFI_SSID_KEY "nvsStaSsid"
#define NVS_STA_ESP_WIFI_PASS_KEY "nvsStaPass"
#define NVS_STA_SHORT_RETRY_NUM_KEY "nvsStaSnum"
#define NVS_STA_LONG_RETRY_TIME_KEY "nvsStaLtime"
#define NVS_STA_LONG_RETRY_NUM_KEY "nvsStaLnum"
#define NVS_STA_AP_DEFAULT_MODE_KEY "nvsApStaMode"
#define NVS_WIFI_MODE_STA "modeSta"
#define NVS_WIFI_MODE_AP "modeAp"

#define NVS_WIFI_RESTART_KEY "Wifi_Restart"
#define NVS_WIFI_RESTART_VALUE_RESTART "restart"
#define NVS_WIFI_RESTART_VALUE_WRITE "write"

#define NVS_STORAGE_NAME "storage"
