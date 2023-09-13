idf_component_register(
    SRCS 
        prv_wifi_connect.c
        prv_http_server.c
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
        prw_wifi_connect.html
)