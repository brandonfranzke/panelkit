/**
 * @file lv_conf.h
 * LVGL configuration for PanelKit
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color: 0: RGB565, 1: BGR565 */
#define LV_COLOR_16_SWAP 0

/* Enable features to draw on transparent background */
#define LV_COLOR_SCREEN_TRANSP 0

/* Adjust color mix functions rounding */
#define LV_COLOR_MIX_ROUND_OFS 0

/* Add alpha byte to RGB565 colors */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*====================
 * MEMORY SETTINGS
 *====================*/

/* Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB) */
#define LV_MEM_SIZE (128U * 1024U)

/* Default size of text buffer (increase for large rendering) */
#define LV_TXT_BUF_SIZE 256

/*====================
 * HAL SETTINGS
 *====================*/

/* Default display refresh period. LVG will redraw changed areas with this period time */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/*====================
 * FEATURE CONFIGURATION
 *====================*/

/*1: Enable the Animations */
#define LV_USE_ANIMATION 1

/* 1: Enable shadow drawing on rectangles */
#define LV_USE_SHADOW 1

/* 1: Enable object opacity (opacity/transparency) */
#define LV_USE_OPA 1

/* 1: Enable image transformations */
#define LV_USE_IMG_TRANSFORM 1

/* 1: Enable anti-aliasing */
#define LV_USE_ANTIALIASING 1

/* 1: Enable multi-language support */
#define LV_USE_BIDI 0

/*1: enable more complex drawing routines */
#define LV_USE_DRAW_COMPLEX 1

/* 1: Enable file system */
#define LV_USE_FS_STDIO 0

/*==================
 * WIDGET USAGE
 *==================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  0
#define LV_USE_CANVAS     0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   0
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       0
#define LV_USE_ROLLER     0
#define LV_USE_SLIDER     0
#define LV_USE_SWITCH     0
#define LV_USE_TEXTAREA   0
#define LV_USE_TABLE      0

/*==================
 * THEMES
 *==================*/

/* A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1

/* Don't compile default theme styles to save Flash */
#define LV_THEME_DEFAULT_DARK 0

/* Allow symbol recoloring */
#define LV_THEME_DEFAULT_GROW 1

/* Use round graphical elements */
#define LV_THEME_DEFAULT_ROUNDED 1

/*===================
 * LAYOUTS
 *==================*/

/* Flex layout */
#define LV_USE_FLEX 1

/* Grid layout */
#define LV_USE_GRID 1

/*==================
 * OTHERS
 *==================*/

/* Change the built in (v)snprintf functions */
#define LV_SPRINTF_CUSTOM 0

/* Decide how many numbers LV_SPRINTF_BUF_SIZE should store */
#define LV_SPRINTF_BUF_SIZE 256

#define LV_MEMCPY_MEMSET_STD 0

#endif /*LV_CONF_H*/