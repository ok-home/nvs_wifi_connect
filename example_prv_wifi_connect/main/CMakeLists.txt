idf_component_register(
    SRCS "example_prv_wifi_connect.c"
    INCLUDE_DIRS "." 
    REQUIRES  prv_wifi_connect esp_http_server driver 
    )
