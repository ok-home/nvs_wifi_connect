idf_component_register(
    SRCS 
        source/prv_wifi_connect.c
        source/prv_http_server.c
    INCLUDE_DIRS 
        include
    PRIV_INCLUDE_DIRS 
        private_include
    #REQUIRES 
       
    PRIV_REQUIRES 
        esp_http_server
        nvs_flash 
        driver 
        esp_wifi
    EMBED_FILES 
        source/prv_wifi_connect.html
)