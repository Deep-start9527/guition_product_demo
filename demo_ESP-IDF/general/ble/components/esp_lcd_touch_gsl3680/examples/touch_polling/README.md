# JC8012P4A1C touch polling

This standalone ESP-IDF 6.1 example prints GSL3680 coordinates using the
existing JC8012P4A1C wiring (SCL 8, SDA 7, reset 22, interrupt 21).
It does not initialize the display, audio or SD card. No PSRAM is needed.
Provide the board's normal touch power supply and I2C pull-ups.

After publishing `guition/esp_lcd_touch_gsl3680` 0.1.0:

```bash
idf.py set-target esp32p4
idf.py build
```

To test an unpublished package, add `override_path` to the main manifest's
GSL dependency, using the location of an extracted component archive.
The published example deliberately has no dependency on parent directories.
When changing IDF versions or chips, remove the previous example build
directory before running `set-target`; board-generated files are not used.

The stock IDF 6.1 ESP32-P4 revision defaults apply. For an older P4 chip,
select the appropriate revision family in menuconfig before building.
Compilation does not prove correct coordinates on a new panel. The driver
uses one global instance and must be called sequentially from one task.
