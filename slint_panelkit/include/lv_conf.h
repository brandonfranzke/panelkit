/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.0+
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* Enable SDL-specific features if needed */
#ifndef LV_USE_SDL
    #define LV_USE_SDL 0
#endif

/*====================
 * COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 32

/*====================
 * MEMORY SETTINGS
 *====================*/
#define LV_MEM_SIZE      (128 * 1024U)
#define LV_MEM_CUSTOM    0

/*=========================
 * RENDERING CONFIGURATION
 *========================*/
/* Default refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD   30

/* Define the default screen size */
#define LV_HOR_RES_MAX            640
#define LV_VER_RES_MAX            480

/*====================
 * FEATURE CONFIGURATION
 *====================*/
#define LV_USE_ANIMATION          1
#define LV_USE_SHADOW             1
#define LV_USE_FLEX               1
#define LV_USE_GRID               1

/*====================
 * WIDGETS
 *====================*/
#define LV_USE_BUTTON             1
#define LV_USE_LABEL              1
#define LV_USE_IMG                1
#define LV_USE_ARC                1
#define LV_USE_LINE               1

/*====================
 * THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT      1
#define LV_THEME_DEFAULT_DARK     0
#define LV_THEME_DEFAULT_GROW     1
#define LV_THEME_DEFAULT_ROUND    1

#endif /* LV_CONF_H */