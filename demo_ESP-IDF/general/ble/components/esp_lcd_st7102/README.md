# ESP LCD ST7102

ST7102 是集成 RGB 显示控制与电容触摸功能的 TDDI 芯片。本组件提供
`esp_lcd_new_panel_st7102()` 和 `esp_lcd_touch_new_i2c_st7102()`。
显示像素由 RGB 总线传输，显示命令由 3-wire SPI panel IO 发送；
触摸使用独立的 I2C panel IO。默认初始化表适用于 480×480 屏幕，
触摸端需要已有可运行的固件。

## jc4848f540 初始化示例

以下程序可作为**独立 ESP-IDF 工程**的 `main/main.c`。引脚、RGB 时序、
双 PSRAM 帧缓冲、触摸地址及背光设置均按当前 jc4848f540 配置填写。运行后循环显示红、绿、蓝三屏，
每屏保持 1 秒；触摸驱动也会完成注册。

工程需要启用 PSRAM，并依赖本地 `esp_lcd_st7102` 组件和
`espressif/esp_lcd_panel_io_additions`。例如在 `main/idf_component.yml` 中声明：

```yaml
dependencies:
  esp_lcd_st7102:
    path: ../components/esp_lcd_st7102
  espressif/esp_lcd_panel_io_additions: "*"
```

```c
#include <stdint.h>
#include <stdbool.h>

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7102.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_WIDTH  480
#define LCD_HEIGHT 480

void app_main(void)
{
    /* GPIO39/48/47: ST7102 display command bus (CS/SCK/SDA). */
    const esp_lcd_panel_io_3wire_spi_config_t display_io_config = {
        .line_config = {
            .cs_io_type = IO_TYPE_GPIO,
            .cs_gpio_num = 39,
            .scl_io_type = IO_TYPE_GPIO,
            .scl_gpio_num = 48,
            .sda_io_type = IO_TYPE_GPIO,
            .sda_gpio_num = 47,
        },
        .expect_clk_speed = 160000,
        .spi_mode = 0,
        .lcd_cmd_bytes = 1,
        .lcd_param_bytes = 1,
        .flags.use_dc_bit = 1,
    };
    esp_lcd_panel_io_handle_t display_io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&display_io_config, &display_io));

    const esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = ST7102_480_480_PANEL_RGB_TIMING(),
        .data_width = 16,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 2,
        .dma_burst_size = 64,
        .hsync_gpio_num = 16,
        .vsync_gpio_num = 17,
        .de_gpio_num = 18,
        .pclk_gpio_num = 21,
        .disp_gpio_num = -1,
        .data_gpio_nums = {4, 5, 6, 7, 15, 8, 20, 3,
                           46, 9, 10, 11, 12, 13, 14, 0},
        .flags.fb_in_psram = 1,
    };
    const st7102_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = (void *)&vendor_config,
    };
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7102(display_io, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    /* GPIO19/45: touch I2C SDA/SCL. The 7-bit touch address is 0x55. */
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 19,
        .scl_io_num = 45,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };
    i2c_master_bus_handle_t touch_bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &touch_bus));

    esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_ST7102_CONFIG();
    touch_io_config.transaction_timeout_ms = 50;
    esp_lcd_panel_io_handle_t touch_io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_bus, &touch_io_config, &touch_io));

    const esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
    };
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_st7102(touch_io, &touch_config, &touch));

    /* GPIO38: backlight LEDC at 20 kHz with 10-bit resolution. */
    const ledc_timer_config_t backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&backlight_timer));
    const ledc_channel_config_t backlight_channel = {
        .gpio_num = 38,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&backlight_channel));

    /* RGB565: red, green, blue. draw_bitmap copies into the panel frame buffer. */
    const uint16_t colors[] = {0xF800, 0x07E0, 0x001F};
    uint16_t *frame = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(frame ? ESP_OK : ESP_ERR_NO_MEM);

    bool backlight_on = false;
    while (1) {
        for (int color = 0; color < 3; ++color) {
            for (int pixel = 0; pixel < LCD_WIDTH * LCD_HEIGHT; ++pixel) {
                frame[pixel] = colors[color];
            }
            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, frame));
            if (!backlight_on) {
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 1023));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
                backlight_on = true;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
```

ESP-IDF 的入口函数是 `app_main()`。已有工程若已定义此函数，请将示例中的初始化和
刷新代码合并到现有入口。示例使用轮询触摸（未设置 INT GPIO）；后续可通过
`esp_lcd_touch_read_data(touch)` 与 `esp_lcd_touch_get_data()` 读取触点。
在独立工程中选择 ESP32-S3 目标后编译、烧录并观察三色循环，例如执行
`idf.py set-target esp32s3`、`idf.py build flash monitor`。
