idf_component_register(
    SRCS
        "main.c"
        "wifi_ap.c"
        "web_server.c"
        "adc_reader.c"
    PRIV_REQUIRES
        esp_wifi nvs_flash esp_http_server esp_adc mdns
    INCLUDE_DIRS "."
    EMBED_TXTFILES
        index.html style.css script.js)
