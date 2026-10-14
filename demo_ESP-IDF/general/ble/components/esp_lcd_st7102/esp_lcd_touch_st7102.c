#include <stdlib.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_touch_st7102.h"

#define ST_REG_FW_VERSION  0x00
#define ST_REG_CONTROL     0x02
#define ST_REG_RESOLUTION  0x05
#define ST_REG_FW_REVISION 0x0C
#define ST_REG_TOUCH       0x10
#define ST_REG_MISC_INFO   0xF0
#define ST_REG_CHIP_ID     0xF4
#define ST_MAX_TOUCHES     10
#define ST_FRAME_SIZE      (4 + 7 * ST_MAX_TOUCHES + 1)

static const char *TAG = "ST7102";

typedef struct {
    esp_lcd_touch_t base; /* First member: public handle and private state share lifetime. */
    SemaphoreHandle_t mutex;
    uint8_t max_touches;
    bool checksum;
    bool sleeping;
    bool rst_configured;
    bool int_configured;
} st7102_t;

static void delay_ms(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    /* Cover conversion rounding and the current partially elapsed tick. */
    vTaskDelay(ticks + 2);
}

static esp_err_t read_reg(esp_lcd_touch_handle_t tp, int reg, void *data, size_t len)
{
    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static void clear_points(esp_lcd_touch_handle_t tp)
{
    portENTER_CRITICAL(&tp->data.lock);
    tp->data.points = 0;
    memset(tp->data.coords, 0, sizeof(tp->data.coords));
    portEXIT_CRITICAL(&tp->data.lock);
}

static esp_err_t st7102_read_data(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "NULL touch handle");
    st7102_t *st = (st7102_t *)tp;
    xSemaphoreTake(st->mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    uint8_t frame[ST_FRAME_SIZE];
    esp_lcd_touch_point_data_t points[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};
    uint8_t count = 0;
    if (st->sleeping) {
        ret = ESP_ERR_INVALID_STATE;
        goto done;
    }
    const size_t len = 4 + 7 * st->max_touches + 1;
    ret = read_reg(tp, ST_REG_TOUCH, frame, len);
    if (ret != ESP_OK) {
        goto done;
    }
    if (frame[0] & 0x80) { /* Firmware requests reset; leave shared reset to the caller. */
        ret = ESP_ERR_INVALID_STATE;
        goto done;
    }
    if (st->checksum) {
        uint8_t sum = 0x5A;
        const size_t check_len = (frame[0] & 0x08) ? len - 1 : 4;
        for (size_t i = 0; i < check_len; ++i) {
            sum = (uint8_t)(sum + frame[i]);
            sum = (uint8_t)((sum << 1) | (sum >> 7));
        }
        if (sum != frame[len - 1]) {
            ret = ESP_ERR_INVALID_CRC;
            goto done;
        }
    }
    /* A frame without coordinates must not reuse old slot contents. */
    if (frame[0] & 0x08) {
        for (uint8_t slot = 0; slot < st->max_touches; ++slot) {
            const uint8_t *p = frame + 4 + 7 * slot;
            if (!(p[0] & 0x80) || count >= CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
                continue;
            }
            points[count++] = (esp_lcd_touch_point_data_t) {
                .track_id = slot,
                .x = ((uint16_t)(p[0] & 0x3F) << 8) | p[1],
                .y = ((uint16_t)(p[2] & 0x3F) << 8) | p[3],
                .strength = p[4],
            };
        }
    }
done:
    portENTER_CRITICAL(&tp->data.lock);
    tp->data.points = ret == ESP_OK ? count : 0;
    memcpy(tp->data.coords, points, sizeof(points));
    portEXIT_CRITICAL(&tp->data.lock);
    xSemaphoreGive(st->mutex);
    return ret;
}

static bool st7102_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                          uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    if (point_num) {
        *point_num = 0;
    }
    ESP_RETURN_ON_FALSE(tp && x && y && point_num && max_point_num, false, TAG, "Invalid coordinate arguments");
    portENTER_CRITICAL(&tp->data.lock);
    *point_num = tp->data.points < max_point_num ? tp->data.points : max_point_num;
    for (uint8_t i = 0; i < *point_num; ++i) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    portEXIT_CRITICAL(&tp->data.lock);
    return *point_num > 0;
}

static esp_err_t st7102_get_track_id(esp_lcd_touch_handle_t tp, uint8_t *ids, uint8_t count)
{
    ESP_RETURN_ON_FALSE(tp && ids && count, ESP_ERR_INVALID_ARG, TAG, "Invalid track ID arguments");
    portENTER_CRITICAL(&tp->data.lock);
    /* A new sample can arrive between get_xy and get_track_id; never read past the cache. */
    if (count > tp->data.points) {
        portEXIT_CRITICAL(&tp->data.lock);
        return ESP_ERR_INVALID_ARG;
    }
    for (uint8_t i = 0; i < count; ++i) {
        ids[i] = tp->data.coords[i].track_id;
    }
    portEXIT_CRITICAL(&tp->data.lock);
    return ESP_OK;
}

static esp_err_t set_sleep(esp_lcd_touch_handle_t tp, bool sleep)
{
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "NULL touch handle");
    st7102_t *st = (st7102_t *)tp;
    xSemaphoreTake(st->mutex, portMAX_DELAY);
    uint8_t control;
    esp_err_t ret = read_reg(tp, ST_REG_CONTROL, &control, 1);
    if (ret == ESP_OK) {
        control = sleep ? control | 0x02 : control & (uint8_t)~0x02;
        ret = esp_lcd_panel_io_tx_param(tp->io, ST_REG_CONTROL, &control, 1);
        if (ret == ESP_OK) {
            st->sleeping = sleep;
            if (!sleep) {
                delay_ms(200);
            }
        }
    }
    clear_points(tp);
    xSemaphoreGive(st->mutex);
    return ret;
}

static esp_err_t st7102_enter_sleep(esp_lcd_touch_handle_t tp)
{
    return set_sleep(tp, true);
}

static esp_err_t st7102_exit_sleep(esp_lcd_touch_handle_t tp)
{
    return set_sleep(tp, false);
}

static esp_err_t st7102_del(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "NULL touch handle");
    st7102_t *st = (st7102_t *)tp;
    if (st->int_configured) {
        gpio_intr_disable(tp->config.int_gpio_num);
        /* Also handles callbacks registered by the caller after construction. */
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
        gpio_reset_pin(tp->config.int_gpio_num);
    }
    if (st->rst_configured) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }
    if (st->mutex) {
        vSemaphoreDelete(st->mutex);
    }
    free(st);
    return ESP_OK;
}

static esp_err_t read_info(st7102_t *st)
{
    uint8_t fw[2], revision[4], geometry[5], misc, chip;
    esp_lcd_touch_handle_t tp = &st->base;
    ESP_RETURN_ON_ERROR(read_reg(tp, ST_REG_FW_VERSION, fw, sizeof(fw)), TAG, "Read firmware/status failed");
    ESP_RETURN_ON_FALSE(!(fw[1] & 0xF0) && (fw[1] & 0x0F) != 2 && (fw[1] & 0x0F) != 6,
                        ESP_ERR_INVALID_STATE, TAG, "Firmware not ready: status=0x%02x", fw[1]);
    ESP_RETURN_ON_ERROR(read_reg(tp, ST_REG_RESOLUTION, geometry, sizeof(geometry)), TAG, "Read geometry failed");
    uint16_t x = ((uint16_t)(geometry[0] & 0x3F) << 8) | geometry[1];
    uint16_t y = ((uint16_t)(geometry[2] & 0x3F) << 8) | geometry[3];
    ESP_RETURN_ON_FALSE(x && y && geometry[4] && geometry[4] <= ST_MAX_TOUCHES,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Invalid geometry: %ux%u, %u slots", x, y, geometry[4]);
    ESP_RETURN_ON_ERROR(read_reg(tp, ST_REG_MISC_INFO, &misc, 1), TAG, "Read capabilities failed");
    ESP_RETURN_ON_ERROR(read_reg(tp, ST_REG_CHIP_ID, &chip, 1), TAG, "Read chip ID failed");
    ESP_RETURN_ON_ERROR(read_reg(tp, ST_REG_FW_REVISION, revision, sizeof(revision)), TAG, "Read revision failed");
    st->max_touches = geometry[4];
    st->checksum = (misc & 0x10) != 0;
    ESP_LOGI(TAG, "ID=0x%02x FW=%02x revision=%02x.%02x.%02x.%02x %ux%u slots=%u checksum=%u",
             chip, fw[0], revision[0], revision[1], revision[2], revision[3],
             x, y, st->max_touches, st->checksum);
    return ESP_OK;
}

esp_err_t esp_lcd_touch_new_i2c_st7102(esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_touch_config_t *config,
                                    esp_lcd_touch_handle_t *out_touch)
{
    ESP_RETURN_ON_FALSE(out_touch, ESP_ERR_INVALID_ARG, TAG, "NULL output handle");
    *out_touch = NULL;
    ESP_RETURN_ON_FALSE(io && config, ESP_ERR_INVALID_ARG, TAG, "NULL IO/config");
    ESP_RETURN_ON_FALSE(config->rst_gpio_num == GPIO_NUM_NC || GPIO_IS_VALID_OUTPUT_GPIO(config->rst_gpio_num),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid reset GPIO");
    ESP_RETURN_ON_FALSE(config->int_gpio_num == GPIO_NUM_NC || GPIO_IS_VALID_GPIO(config->int_gpio_num),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid interrupt GPIO");
    ESP_RETURN_ON_FALSE(config->rst_gpio_num == GPIO_NUM_NC || config->rst_gpio_num != config->int_gpio_num,
                        ESP_ERR_INVALID_ARG, TAG, "Reset and interrupt GPIO must differ");
    ESP_RETURN_ON_FALSE(!config->interrupt_callback || config->int_gpio_num != GPIO_NUM_NC,
                        ESP_ERR_INVALID_ARG, TAG, "Callback requires interrupt GPIO");
    st7102_t *st = calloc(1, sizeof(*st));
    ESP_RETURN_ON_FALSE(st, ESP_ERR_NO_MEM, TAG, "No memory");
    esp_lcd_touch_handle_t tp = &st->base;
    tp->io = io;
    tp->config = *config;
    /* Register the callback only after the controller is ready. */
    tp->config.interrupt_callback = NULL;
    tp->data.lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    tp->read_data = st7102_read_data;
    tp->get_xy = st7102_get_xy;
    tp->get_track_id = st7102_get_track_id;
    tp->enter_sleep = st7102_enter_sleep;
    tp->exit_sleep = st7102_exit_sleep;
    tp->del = st7102_del;
    esp_err_t ret = ESP_ERR_NO_MEM;
    st->mutex = xSemaphoreCreateMutex();
    if (!st->mutex) {
        goto fail;
    }
    if (config->rst_gpio_num != GPIO_NUM_NC) {
        gpio_config_t gpio = {
            .pin_bit_mask = 1ULL << config->rst_gpio_num,
            .mode = GPIO_MODE_OUTPUT,
        };
        ret = gpio_config(&gpio);
        if (ret != ESP_OK) {
            goto fail;
        }
        st->rst_configured = true;
        ret = gpio_set_level(config->rst_gpio_num, config->levels.reset);
        if (ret != ESP_OK) {
            goto fail;
        }
        delay_ms(1);
        ret = gpio_set_level(config->rst_gpio_num, !config->levels.reset);
        if (ret != ESP_OK) {
            goto fail;
        }
        delay_ms(100);
    }
    ret = read_info(st);
    if (ret != ESP_OK) {
        goto fail;
    }
    if (config->int_gpio_num != GPIO_NUM_NC) {
        gpio_config_t gpio = {
            .pin_bit_mask = 1ULL << config->int_gpio_num,
            .mode = GPIO_MODE_INPUT,
            .intr_type = config->levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE,
        };
        ret = gpio_config(&gpio);
        if (ret != ESP_OK) {
            goto fail;
        }
        st->int_configured = true;
        ret = gpio_intr_disable(config->int_gpio_num);
        if (ret != ESP_OK) {
            goto fail;
        }
        if (config->interrupt_callback) {
            ret = esp_lcd_touch_register_interrupt_callback(tp, config->interrupt_callback);
            if (ret != ESP_OK) {
                goto fail;
            }
        }
    }
    *out_touch = tp;
    return ESP_OK;
fail:
    st7102_del(tp);
    return ret;
}
