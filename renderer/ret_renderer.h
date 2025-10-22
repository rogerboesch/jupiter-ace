#ifndef RET_RENDERER_H
#define RET_RENDERER_H

#include "rb_types.h"

#include "ret_palette.h"
#include "ret_font.h"

#ifdef IPHONE_OS
	#define RET_PIXEL_WIDTH_RET 200
	#define RET_PIXEL_HEIGHT_RET 200
#else
	#define RET_PIXEL_WIDTH_RET 480
	#define RET_PIXEL_HEIGHT_RET 640
#endif

#define RET_PIXEL_WIDTH_C64 320
#define RET_PIXEL_HEIGHT_C64 200
#define RET_PIXEL_WIDTH_ATARIXL 320
#define RET_PIXEL_HEIGHT_ATARIXL 192
#define RET_PIXEL_WIDTH_VECTREX 440
#define RET_PIXEL_HEIGHT_VECTREX 546
#define RET_PIXEL_WIDTH_SPECCY 256
#define RET_PIXEL_HEIGHT_SPECCY 192
#define RET_PIXEL_WIDTH_TEST 480
#define RET_PIXEL_HEIGHT_TEST 320

#define RET_DEFAULT_PIXEL_WIDTH RET_PIXEL_WIDTH_RET
#define RET_DEFAULT_PIXEL_HEIGHT RET_PIXEL_HEIGHT_RET

#define RET_PIXEL_CENTER_X 0
#define RET_PIXEL_CENTER_Y 0

void ret_rend_set_pixel(int x, int y, BYTE paletteColor, BYTE brightness);
int ret_rend_get_pixel_internal(int x, int y);
void ret_rend_clear_screen(void);

void ret_rend_draw_char(int x, int y, const unsigned char ch, BOOL invert, BYTE paletteColor);
void ret_rend_scroll_up(int height);

void ret_rend_initialize(int width, int height);
ret_color ret_rend_get_bg_color(void);
ret_color ret_rend_get_border_color(void);

BYTE RETSetBorderColor(BYTE index);
BYTE RETSetBgColor(BYTE index);
BYTE RETGetBgColor(void);
BYTE RETSetFgColor(BYTE index);
BYTE RETGetFgColor(void);
BYTE RETSetBrightness(BYTE index);
BYTE RETGetBrightness(void);
BYTE RETSetDotMode(BYTE mode);

void RETMoveTo(int x, int y);
void RETMoveBy(int x, int y);
void RETDrawPixel(int x, int y);
void RETDrawLine(int x1, int y1, int x2, int y2);
void RETDrawLineTo(int x1, int y1);
void RETDrawCircle(int xc, int yc, int r);
void RETRenderFrame(void);

void RETSetScreenSize(int width, int height);
int RETGetScreenWidth(void);
int RETGetScreenHeight(void);

void RETDrawVString(char* str, int x, int y);

#endif
