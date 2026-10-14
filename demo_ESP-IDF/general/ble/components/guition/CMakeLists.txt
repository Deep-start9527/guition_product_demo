# Board setup sources are selected and compiled by BMGR's generated component.
# Do not glob all board directories: factory entry points have identical names.
if(CONFIG_GUITION_BOARD_NO_PSRAM AND CONFIG_SPIRAM)
    message(FATAL_ERROR "This Guition ESP32/C3 board has no PSRAM. Disable CONFIG_SPIRAM.")
endif()
idf_component_register(
    SRCS "src/guition_touch.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_board_manager esp_lcd_touch esp_lcd esp_driver_spi esp_driver_i2c
)
