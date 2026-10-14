idf_component_register(
    SRCS "esp_lcd_touch_gsl3680.c" "gsl_point_id.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_include"
    REQUIRES esp_lcd
    PRIV_REQUIRES esp_driver_gpio freertos heap log esp_common
)
