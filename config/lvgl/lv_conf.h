/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.3.11
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface */
#define LV_COLOR_16_SWAP 0

/* Enable features */
#define LV_USE_LOG                     1
#define LV_LOG_LEVEL                   LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF                  1
#define LV_USE_MEMCPY_HEAP_BUF         1
#define LV_MEM_SIZE                    (128U * 1024U)
#define LV_MEM_CUSTOM                  0
#define LV_ENABLE_GC                   0
#define LV_SPRINTF_CUSTOM              0
#define LV_SPRINTF_BUF_SIZE            256

/* Input device settings */
#define LV_INDEV_DEF_READ_PERIOD       30
#define LV_INDEV_DEF_DRAG_LIMIT        10
#define LV_INDEV_DEF_DRAG_THROW        10
#define LV_INDEV_DEF_LONG_PRESS_TIME   400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100
#define LV_INDEV_DEF_GESTURE_LIMIT     50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3

/* Font usage */
#define LV_FONT_DEFAULT              &lv_font_montserrat_14
#define LV_FONT_MONTSERRAT_8         1
#define LV_FONT_MONTSERRAT_10        1
#define LV_FONT_MONTSERRAT_12        1
#define LV_FONT_MONTSERRAT_14        1
#define LV_FONT_MONTSERRAT_16        1
#define LV_FONT_MONTSERRAT_18        1
#define LV_FONT_MONTSERRAT_20        1
#define LV_FONT_MONTSERRAT_22        1
#define LV_FONT_MONTSERRAT_24        1
#define LV_FONT_MONTSERRAT_26        1
#define LV_FONT_MONTSERRAT_28        1
#define LV_FONT_MONTSERRAT_30        1
#define LV_FONT_MONTSERRAT_32        1
#define LV_FONT_MONTSERRAT_34        1
#define LV_FONT_MONTSERRAT_36        1
#define LV_FONT_MONTSERRAT_38        1
#define LV_FONT_MONTSERRAT_40        1
#define LV_FONT_MONTSERRAT_42        1
#define LV_FONT_MONTSERRAT_44        1
#define LV_FONT_MONTSERRAT_46        1
#define LV_FONT_MONTSERRAT_48        1

/* Enable various widgets */
#define LV_USE_ANIMATION              1
#define LV_USE_SHADOW                 1
#define LV_SHADOW_CACHE_SIZE          0
#define LV_USE_OUTLINE                1
#define LV_USE_PATTERN                1
#define LV_USE_VALUE_STR              1
#define LV_USE_BLEND_MODES            1
#define LV_USE_OPA_SCALE              1
#define LV_USE_IMG_TRANSFORM          1
#define LV_USE_IMG_CACHE_MANAGER      0
#define LV_IMG_CACHE_DEF_SIZE         0
#define LV_USE_LABEL                  1
#define LV_LABEL_TEXT_SELECTION       1
#define LV_LABEL_LONG_TXT_HINT        1
#define LV_USE_LINE                   1
#define LV_USE_ARC                    1
#define LV_USE_DROPDOWN               1
#define LV_USE_IMG                    1
#define LV_USE_BAR                    1
#define LV_USE_SLIDER                 1
#define LV_USE_BTNMATRIX              1
#define LV_USE_CHECKBOX               1
#define LV_USE_LIST                   1
#define LV_USE_MENU                   1
#define LV_USE_MSGBOX                 1
#define LV_USE_ROLLER                 1
#define LV_USE_TEXTAREA               1
#define LV_USE_CANVAS                 1
#define LV_USE_SWITCH                 1

/* Animations */
#define LV_USE_FLEX                   1
#define LV_USE_GRID                   1

/* Themes */
#define LV_USE_THEME_DEFAULT          1
#define LV_THEME_DEFAULT_DARK         0
#define LV_THEME_DEFAULT_GROW         0
#define LV_THEME_DEFAULT_TRANSITION_TIME 80

#endif /* LV_CONF_H */