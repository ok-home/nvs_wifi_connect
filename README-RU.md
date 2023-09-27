[En](/README.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# Выбор режимов (AP/STA,SSID/PASS) и подключение к WIFI
 - Подключается как компонент к вашей программе
 - Данные хранятся в NVS
 - Настройка параметров WIFI через WEB интерфейс
 - Первое подключение через softAP
 - Последующие подключения - по данным записанным в NVS
 - Интерфейс -> nvs_wifi_connect.h
 - Пример  -> example_nvs_wifi_connect.c
