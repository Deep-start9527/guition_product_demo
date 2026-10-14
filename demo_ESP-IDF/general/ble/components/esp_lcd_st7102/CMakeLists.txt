set(REQ esp_lcd)
idf_build_get_property(component_manager IDF_COMPONENT_MANAGER)
if(NOT component_manager)
    list(APPEND REQ esp_lcd_touch)
endif()
idf_component_register(SRCS "esp_lcd_touch_st7102.c" "esp_lcd_st7102.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ${REQ})
