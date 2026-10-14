# ESP LCD AXS15260

本组件为 AXS15260/AXS15260D 提供 RGB 显示和 I2C 触摸驱动。像素通过 RGB 总线输出，显示寄存器通过 3-wire SPI panel IO 配置，触摸报告通过 I2C 读取。


公开接口为 `esp_lcd_new_panel_axs15260()` 和 `esp_lcd_touch_new_i2c_axs15260()`。默认 RGB 时序宏为 `AXS15260_540_540_PANEL_RGB_TIMING()`，触摸 I2C 地址为 `ESP_LCD_TOUCH_IO_I2C_AXS15260_ADDRESS`（7-bit 地址 `0x3B`）。

## 依赖

在 `main/idf_component.yml` 中声明本地组件和 3-wire panel IO 扩展：

```yaml
dependencies:
  esp_lcd_axs15260:
    path: ../components/esp_lcd_axs15260
  espressif/esp_lcd_panel_io_additions: "*"
```

## 初始化示例

以下 GPIO、时序和 I2C 总线配置仅供参考。其它板卡应替换为自身的引脚和 RGB 参数。

```c
#include "driver/i2c_master.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_axs15260.h"
#include "esp_lcd_touch_axs15260.h"

#define LCD_WIDTH  540
#define LCD_HEIGHT 540

void app_main(void)
{
    /* LCD command pins: CS=38, SCL=61, SDA=60. SDA is reused as TP_INT. */
    const esp_lcd_panel_io_3wire_spi_config_t display_io_config = {
        .line_config = {
            .cs_io_type = IO_TYPE_GPIO, .cs_gpio_num = 38,
            .scl_io_type = IO_TYPE_GPIO, .scl_gpio_num = 61,
            .sda_io_type = IO_TYPE_GPIO, .sda_gpio_num = 60,
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
        .timings = AXS15260_540_540_PANEL_RGB_TIMING(),
        .data_width = 16,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 3,
        .dma_burst_size = 64,
        .hsync_gpio_num = 44,
        .vsync_gpio_num = 45,
        .de_gpio_num = 43,
        .pclk_gpio_num = 40,
        .disp_gpio_num = -1,
        .data_gpio_nums = {33, 19, 35, 34, 36, 13, 14, 15,
                           16, 18, 17, 8, 9, 10, 11, 12},
        .flags.fb_in_psram = 1,
    };
    const axs15260_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        /* NULL selects the built-in 540x540 register sequence. */
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = (void *)&vendor_config,
    };
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15260(display_io, &panel_config, &panel));
    /* Creation sends the display commands and deletes display_io. */
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 0,
        .scl_io_num = 1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };
    i2c_master_bus_handle_t touch_bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &touch_bus));

    esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_AXS15260_CONFIG();
    esp_lcd_panel_io_handle_t touch_io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_bus, &touch_io_config, &touch_io));
    const esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = 60,
        .levels.interrupt = 0,
    };
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_axs15260(touch_io, &touch_config, &touch));

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

`esp_lcd_new_panel_axs15260()` 会在创建过程中发送寄存器初始化表并删除传入的显示 IO。调用成功后不能再使用 `display_io`；这种生命周期也让显示 SDA 可在初始化后交给触摸中断使用。板级管理器配置 `auto_del_panel_io: true` 时与此约定匹配。

示例未配置触摸中断回调；可用 `esp_lcd_touch_read_data(touch)` 轮询触摸状态，再用 `esp_lcd_touch_get_data()` 读取坐标。设置中断回调时，在触摸配置中填写 `interrupt_callback` 和对应的有效电平。
