#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "lcd.h"
#include "pingcfg.h"

#define TAG "lcd"

#define HW_LDO_MIPI_CHAN                        (3)
#define HW_LDO_MIPI_VOLTAGE_MV                  (2500)
#define HW_LCD_BIT_PER_PIXEL                    (16)
#define HW_MIPI_DPI_PX_FORMAT                   (LCD_COLOR_PIXEL_FORMAT_RGB565)

#define HW_I2C_NUM                          (I2C_NUM_0)
#define HW_I2C_CLK_SPEED_HZ                 (400000)

#define TOUCH_ROTATION_TYPE             TOUCH_ROTATION_MIPI_DSI


static esp_lcd_panel_io_handle_t s_mipi_dbi_io;
static esp_lcd_panel_handle_t s_panel_handle;

static i2c_master_bus_handle_t s_touch_i2c_bus = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static esp_lcd_panel_io_handle_t s_touch_io_handle = NULL;

static esp_err_t hw_touch_i2c_init(void)
{
    if (s_touch_i2c_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .i2c_port = HW_I2C_NUM,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &s_touch_i2c_bus));

    return ESP_OK;
}
void lcd_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle, esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode, esp_lv_adapter_rotation_t rotation)
{
   ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .num_fbs = 1,
        .bounce_buffer_size_px = 10 * HW_LCD_V_RES,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            // 以下参数请参考LCD规格书
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .flags.pclk_active_neg = true,
        },
        .flags.fb_in_psram = true, // 在 PSRAM 中分配帧缓冲区
    };
    panel_config.num_fbs = esp_lv_adapter_get_required_frame_buffer_count(tear_avoid_mode, rotation);
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));

    *panel_handle = s_panel_handle;
    if (io_handle) {
        *io_handle = s_mipi_dbi_io;
    }

}
void touch_init(esp_lcd_touch_handle_t *ret_touch, esp_lv_adapter_rotation_t rotation)
{
    hw_touch_i2c_init();

    bool swap_xy = false;
    bool mirror_x = false;
    bool mirror_y = false;
    // touch_get_rotation_flags(TOUCH_ROTATION_TYPE, rotation, &swap_xy, &mirror_x, &mirror_y);

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
    };

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_err_t ret = ESP_OK;

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = HW_I2C_CLK_SPEED_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(s_touch_i2c_bus, &tp_io_config, &tp_io_handle));
    ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, ret_touch);

    if (ret != ESP_OK) {
        if (tp_io_handle) {
            esp_lcd_panel_io_del(tp_io_handle);
        }
        return;
    }

    s_touch_handle = *ret_touch;
    s_touch_io_handle = tp_io_handle;
}

void lcd_backlight_init(void)
{
     // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = LCD_BACKLIGHT_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
}
void lcd_backlight_on(void)
{
    return lcd_backlight_set_level(100);
}
void lcd_backlight_off(void)
{
    return lcd_backlight_set_level(0);
}

//level 0-100
void lcd_backlight_set_level(int brightness_percent)
{
     if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
    uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    return;
}