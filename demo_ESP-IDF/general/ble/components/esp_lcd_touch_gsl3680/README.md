# GSL3680 touch driver

An ESP-IDF `esp_lcd_touch` driver for the GSL3680 panel used by Guition
JC8012P4A1C. Registry name: `guition/esp_lcd_touch_gsl3680`, version `0.1.0`.
Initial support is ESP32-P4, ESP-IDF 6.1.x and `espressif/esp_lcd_touch` 1.2.1.
The driver has no Arduino, LVGL or Board Manager dependency.

The bundled firmware and calibration data are panel-specific. A different
GSL3680 panel may need different firmware; this package does not claim
compatibility merely because the controller has the same name.

## Installation

After publication, add to the consuming component's `idf_component.yml`:

```yaml
dependencies:
  guition/esp_lcd_touch_gsl3680: "==0.1.0"
```

During development, add `override_path` to that dependency, pointing to an
extracted local copy of this package. Remove the override after publication.
An offline archive build validates the component, but does not establish
that the version has been uploaded to the registry.

## API and ownership

Include `esp_lcd_touch_gsl3680.h`, create an I2C bus and an I2C panel IO with
`ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG()`, then call
`esp_lcd_touch_new_i2c_gsl3680(io, &config, &touch)`.
Use standard `esp_lcd_touch_read_data()` and
`esp_lcd_touch_get_coordinates()` to read touch events.
The address is **7-bit 0x40**; 0x80 is the 8-bit write address.
The default IO macro uses the existing board's 100 kHz clock.

For JC8012P4A1C use SCL 8, SDA 7, reset 22, interrupt 21, 800×1280 and
`mirror_x = true`. These parameters match the existing IDF port. The
standalone polling example is under `examples/touch_polling`.

The caller owns the bus and panel IO. On shutdown call `esp_lcd_touch_del()`
first, then `esp_lcd_panel_io_del()` and finally delete the bus, unless
another device still uses that bus. When Board Manager owns the bus, return
its reference through Board Manager instead of deleting the bus directly.

The original algorithm and driver keep global state. Use **one GSL3680
instance**, with sequential calls from one task; concurrent polling or
multiple panels are not supported. `esp_while_read()` remains available
for compatibility with existing consumers and requires a live instance.
The original initialization and I2C error handling are retained; this
packaging release does not constitute a runtime robustness rewrite.

## Pack and publish

Activate ESP-IDF 6.1, change to this component directory and run:

```bash
compote component pack --name esp_lcd_touch_gsl3680 --dest-dir dist
```

The equivalent CLI is `python -m idf_component_manager component pack`.
Inspect the archive and complete the licensing information below. Then,
using your own registry profile/token, publish with:

```bash
compote component upload --name esp_lcd_touch_gsl3680 --namespace guition
```

The upload command is documentation only; the migration tools do not upload.
Set a real public repository URL and maintainer contact if available;
no placeholder URL or fabricated identity is included in the manifest.
See the [official component file reference](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/component_files.html)
and [manifest reference](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html).

## License status

The supplied `gsl_point_id.c` explicitly declares **GPL-2.0-or-later**;
its notice and full license text are retained. The other original files
and controller firmware arrived without an explicit license statement.
`LICENSE` and `NOTICE` record that distinction; they do not relicense
unknown upstream material. The publisher must supply/confirm those terms
before upload and then complete the manifest's `license` field as appropriate.
Technical packing success alone does not resolve this missing information.
