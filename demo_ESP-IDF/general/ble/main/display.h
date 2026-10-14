#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t display_init(void);
bool lvgl_display_lock(int timeout_ms);
void lvgl_display_unlock(void);
bool display_has_touch(void);
