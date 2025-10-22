
#ifndef ret_palette_h
#define ret_palette_h

#include "rb_types.h"
#include <stdint.h>

#define RET_PALETTE_SIZE 8
#define RET_BRIGHTNESS_NORMAL 0

#define RET_COLOR_BLACK        0
#define RET_COLOR_BLUE         1
#define RET_COLOR_RED          2
#define RET_COLOR_MAGENTA      3
#define RET_COLOR_GREEN        4
#define RET_COLOR_CYAN         5
#define RET_COLOR_YELLOW       6
#define RET_COLOR_WHITE        7

typedef uint32_t ARGB_color;

typedef union {
    ARGB_color argb;
    struct {
        uint8_t a;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
} ARGB;

typedef struct ret_color_ {
	BYTE r, g, b;
} ret_color;

ret_color RETPaletteGetColor(BYTE index);
ret_color RETPaletteGetColorWithBrightness(BYTE index, BYTE bindex);

#endif
