/* SPDX-License-Identifier: MIT */
#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_io_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GUITION_TOUCH_MODE_NONE,
    GUITION_TOUCH_MODE_CAPACITIVE,
    GUITION_TOUCH_MODE_RESISTIVE,
} guition_touch_mode_t;

typedef enum {
    GUITION_TOUCH_BUS_I2C,
    GUITION_TOUCH_BUS_SPI,
} guition_touch_bus_t;

typedef esp_err_t (*guition_touch_factory_t)(esp_lcd_panel_io_handle_t io,
                                            const esp_lcd_touch_config_t *config,
                                            esp_lcd_touch_handle_t *out_touch);

/** Board setup supplies the selected variant; bus_name identifies a BMGR
 * peripheral. LCD and touch must use the SAME name when sharing a SPI bus.
 * The name string must remain valid until guition_touch_destroy completes.
 * No bus is initialized outside BMGR, including a dedicated touch bus.
 */
typedef struct {
    guition_touch_mode_t mode;
    guition_touch_bus_t bus;
    const char *bus_name;
    union {
        esp_lcd_panel_io_i2c_config_t i2c;
        esp_lcd_panel_io_spi_config_t spi;
    } io;
    esp_lcd_touch_config_t touch;
    guition_touch_factory_t factory;
} guition_touch_config_t;

typedef struct guition_touch_device guition_touch_device_t;

/** Returns the menuconfig choice, including NONE for a non-Guition board. */
guition_touch_mode_t guition_touch_selected_mode(void);

/** Create a custom BMGR touch device. NONE accepts NULL config and returns a
 * non-NULL lifecycle object, with no driver, IO or bus reference. A factory
 * must leave out_touch NULL on failure or return a deletable partial handle.
 * On init failure, out_device is normally NULL; if cleanup also fails, it is
 * non-NULL and MUST be retained and passed to destroy until cleanup succeeds.
 * Initialize/deinitialize from one task, matching BMGR's lifecycle contract.
 */
esp_err_t guition_touch_create(const guition_touch_config_t *config,
                              guition_touch_device_t **out_device);

/** Delete driver and IO, then return the bus reference. On cleanup failure
 * the object remains valid for retry. Never directly frees a shared bus.
 */
esp_err_t guition_touch_destroy(guition_touch_device_t *device);

/** NULL for a disabled touch lifecycle object or NULL device. */
esp_lcd_touch_handle_t guition_touch_get_handle(const guition_touch_device_t *device);

/** BMGR custom init callback bridge. Unlike the direct create API, this
 * always leaves out_device NULL on error, since BMGR discards failed custom
 * init outputs. A cleanup-failed object is retained internally and retried
 * before the next BMGR create; only one board touch device is supported.
 */
esp_err_t guition_touch_bmgr_create(const guition_touch_config_t *config, void **out_device);

/** Signature matches a BMGR custom device deinit callback. */
esp_err_t guition_touch_bmgr_destroy(void *device);

/** Retry cleanup retained after a failed BMGR init. Call from the error or
 * shutdown path if no further init will be attempted. Serialized with create.
 */
esp_err_t guition_touch_bmgr_recover(void);

#ifdef __cplusplus
}
#endif
