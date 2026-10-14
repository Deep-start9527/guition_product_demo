# JC8012P4A1

JC8012P4A1C_I_W_Y 存在有两种屏幕，本配置默认使用LCD_V2, 如果发现屏幕显示异常或者屏幕已经亮起但是显示黑色，请及时断电并且排除日志，倘若日志正常请切换配置，

## 切换配置
1. 修改`setup_device.c`
``` C
#if __has_include(<esp_lcd_jd9365.h>)
#define HAS_JD9365  1
#define USE_LCD_V1      1           
#define USE_LCD_V2      0
#include "esp_lcd_jd9365.h"
#endif  /* __has_include(<esp_lcd_ek79007.h>) */
```

2. 修改`board_devices.yaml
``` yaml
    dpi_clock_freq_mhz: 60

    video_timing:
        h_size: 800
        v_size: 1280
        hsync_back_porch : 20
        hsync_pulse_width : 20
        hsync_front_porch : 40
        vsync_back_porch : 8
        vsync_pulse_width : 4
        vsync_front_porch : 20
```

## 芯片版低于3.x时在menuconfig中设置`ESP32P4_SELECTS_REV_LESS_V3`