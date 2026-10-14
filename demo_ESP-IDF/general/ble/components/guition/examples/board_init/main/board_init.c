/* SPDX-License-Identifier: MIT */
#include "sdkconfig.h"
#include "esp_board_manager.h"
#include "esp_err.h"

void app_main(void)
{
    /* Obtain only display resources. Do not initialize every declared bus. */
    ESP_ERROR_CHECK(esp_board_manager_init_device_by_name("display_lcd"));
    if (esp_board_manager_check_name("lcd_touch")) {
        ESP_ERROR_CHECK(esp_board_manager_init_device_by_name("lcd_touch"));
    }
    if (esp_board_manager_check_name("lcd_brightness")) {
        ESP_ERROR_CHECK(esp_board_manager_init_device_by_name("lcd_brightness"));
    }
}
