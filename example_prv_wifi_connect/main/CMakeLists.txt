idf_component_register(
    SRCS 
        "example_prv_wifi_connect.c"
        "example_echo_ws_server.c"
    INCLUDE_DIRS "." 
    REQUIRES  
        prv_Wifi_Connect
        esp_http_server 
        esp_wifi
        mdns
    EMBED_FILES 
        example_echo_ws_server.html
    )
