set(REQ "esp_lcd")

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.3")
    list(APPEND REQ esp_driver_gpio)
else()
    list(APPEND REQ driver)
endif()

idf_build_get_property(idf_component_manager IDF_COMPONENT_MANAGER)
if(NOT idf_component_manager)
    list(APPEND REQ esp_lcd_touch)
endif()

idf_component_register(
    SRCS "esp_lcd_touch_axs15260.c" "esp_lcd_axs15260.c"
    INCLUDE_DIRS "include"
    REQUIRES ${REQ}
)
