idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "." 
    REQUIRES esp_http_server nvs_flash driver esp_wifi protocol_examples_common prv_wifi_connect
    #EMBED_FILES "prw_wifi_connect.html"
    )
