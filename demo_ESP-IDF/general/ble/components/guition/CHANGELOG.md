# Changelog

## 0.1.0 — migration preview

- Add 17 ESP32/C3/S3 board definitions, configurable touch and shared SPI.
- Preserve the existing hardware-verified ESP32-P4 board parameters.
- Use the external `guition/esp_lcd_touch_gsl3680` 0.1.0 dependency for JC8012.
- Map both JC1060 Arduino SKUs to the existing `jc1060p470` BMGR definition.
- Add a standalone initialization example and registry archive verification.
- Hardware validation of new ESP32/C3/S3 configurations remains pending.
