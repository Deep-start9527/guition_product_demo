set(app_requires bt nvs_flash esp_timer guition)
if(CONFIG_IDF_TARGET_ESP32P4)
    list(APPEND app_requires esp_hosted esp_wifi_remote)
endif()
idf_component_register(SRCS "main.c" "display.c" "app_ui.c" "app_service.c" "ble_policy.c"
    INCLUDE_DIRS "." REQUIRES ${app_requires})
