[Ru](/README-RU.md)

| Supported Targets |
| ESP32 ESP32S3 ESP32C3 |
| ----------------- |

# Select modes (AP/STA,SSID/PASS) and connect to WIFI
  - Connects as a component to your program
  - Data is stored in NVS
  - Configuring WIFI parameters via WEB interface
  - First connection via softAP
  - Subsequent connections - according to data recorded in NVS
  - Interface -> the prv_wifi_connect.h 
  - Example   -> prv_example_connect.c