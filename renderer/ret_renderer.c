
#include "ret_renderer.h"
#include "ret_textbuffer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "ret_font.h"

extern void platform_render_frame(BYTE* data, BYTE border_color, int index, int width, int height);
extern BYTE ret_font_speccy[];

#define RET_BUF_SIZE RET_PIXEL_WIDTH*RET_PIXEL_HEIGHT*4

typedef struct _RETScreen {
    BYTE border_color;			// Palette color border
    BYTE bg_color, fg_color;    // Palette colors
    BYTE brightness;         	// Brightness 0|1
    BYTE mode_dotted;           // Dotted mode for lines (0=off, 1-255=pattern)
    BOOL dirty_bit;          // Flags if screen is updated

    BYTE* data;                 // Pixel data buffer
    int width;                  // Screen width
    int height;                 // Screen height

    int cursorx;                // Graphics cursor
    int cursory;
	
	int direct_render;			// 0=Needs render frame, 1=Output every character
} RETScreen;

static RETScreen ret_screen_main;

// -----------------------------------------------------------------------------
#pragma mark - Vector font table

#define P(x,y) ((((x) & 0xF) << 4) | (((y) & 0xF) << 0))
#define FONT_UP 0xFE
#define FONT_LAST 0xFF

const BYTE ret_vecfont[128][8] = {
	['0' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), P(8,12), FONT_LAST },
	['1' - 0x20] = { P(4,0), P(4,12), P(3,10), FONT_LAST },
	['2' - 0x20] = { P(0,12), P(8,12), P(8,7), P(0,5), P(0,0), P(8,0), FONT_LAST },
	['3' - 0x20] = { P(0,12), P(8,12), P(8,0), P(0,0), FONT_UP, P(0,6), P(8,6), FONT_LAST },
	['4' - 0x20] = { P(0,12), P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0), FONT_LAST },
	['5' - 0x20] = { P(0,0), P(8,0), P(8,6), P(0,7), P(0,12), P(8,12), FONT_LAST },
	['6' - 0x20] = { P(0,12), P(0,0), P(8,0), P(8,5), P(0,7), FONT_LAST },
	['7' - 0x20] = { P(0,12), P(8,12), P(8,6), P(4,0), FONT_LAST },
	['8' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), FONT_UP, P(0,6), P(8,6), },
	['9' - 0x20] = { P(8,0), P(8,12), P(0,12), P(0,7), P(8,5), FONT_LAST },
	[' ' - 0x20] = { FONT_LAST },
	['.' - 0x20] = { P(3,0), P(4,0), FONT_LAST },
	[',' - 0x20] = { P(2,0), P(4,2), FONT_LAST },
	['-' - 0x20] = { P(2,6), P(6,6), FONT_LAST },
	['+' - 0x20] = { P(1,6), P(7,6), FONT_UP, P(4,9), P(4,3), FONT_LAST },
	['!' - 0x20] = { P(4,0), P(3,2), P(5,2), P(4,0), FONT_UP, P(4,4), P(4,12), FONT_LAST },
	['#' - 0x20] = { P(0,4), P(8,4), P(6,2), P(6,10), P(8,8), P(0,8), P(2,10), P(2,2) },
	['^' - 0x20] = { P(2,6), P(4,12), P(6,6), FONT_LAST },
	['=' - 0x20] = { P(1,4), P(7,4), FONT_UP, P(1,8), P(7,8), FONT_LAST },
	['*' - 0x20] = { P(0,0), P(4,12), P(8,0), P(0,8), P(8,8), P(0,0), FONT_LAST },
	['_' - 0x20] = { P(0,0), P(8,0), FONT_LAST },
	['/' - 0x20] = { P(0,0), P(8,12), FONT_LAST },
	['\\' - 0x20] = { P(0,12), P(8,0), FONT_LAST },
	['@' - 0x20] = { P(8,4), P(4,0), P(0,4), P(0,8), P(4,12), P(8,8), P(4,4), P(3,6) },
	['$' - 0x20] = { P(6,2), P(2,6), P(6,10), FONT_UP, P(4,12), P(4,0), FONT_LAST },
	['&' - 0x20] = { P(8,0), P(4,12), P(8,8), P(0,4), P(4,0), P(8,4), FONT_LAST },
	['[' - 0x20] = { P(6,0), P(2,0), P(2,12), P(6,12), FONT_LAST },
	[']' - 0x20] = { P(2,0), P(6,0), P(6,12), P(2,12), FONT_LAST },
	['(' - 0x20] = { P(6,0), P(2,4), P(2,8), P(6,12), FONT_LAST },
	[')' - 0x20] = { P(2,0), P(6,4), P(6,8), P(2,12), FONT_LAST },
	['{' - 0x20] = { P(6,0), P(4,2), P(4,10), P(6,12), FONT_UP, P(2,6), P(4,6), FONT_LAST },
	['}' - 0x20] = { P(4,0), P(6,2), P(6,10), P(4,12), FONT_UP, P(6,6), P(8,6), FONT_LAST },
	['%' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(2,10), P(2,8), FONT_UP, P(6,4), P(6,2) },
	['<' - 0x20] = { P(6,0), P(2,6), P(6,12), FONT_LAST },
	['>' - 0x20] = { P(2,0), P(6,6), P(2,12), FONT_LAST },
	['|' - 0x20] = { P(4,0), P(4,5), FONT_UP, P(4,6), P(4,12), FONT_LAST },
	[':' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(4,3), FONT_LAST },
	[';' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(1,2), FONT_LAST },
	['"' - 0x20] = { P(2,10), P(2,6), FONT_UP, P(6,10), P(6,6), FONT_LAST },
	['\'' - 0x20] = { P(2,6), P(6,10), FONT_LAST },
	['`' - 0x20] = { P(2,10), P(6,6), FONT_LAST },
	['~' - 0x20] = { P(0,4), P(2,8), P(6,4), P(8,8), FONT_LAST },
	['?' - 0x20] = { P(0,8), P(4,12), P(8,8), P(4,4), FONT_UP, P(4,1), P(4,0), FONT_LAST },
	['A' - 0x20] = { P(0,0), P(0,8), P(4,12), P(8,8), P(8,0), FONT_UP, P(0,4), P(8,4) },
	['B' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,10), P(4,6), P(8,2), P(4,0), P(0,0) },
	['C' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
	['D' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,8), P(8,4), P(4,0), P(0,0), FONT_LAST },
	['E' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
	['F' - 0x20] = { P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
	['G' - 0x20] = { P(6,6), P(8,4), P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
	['H' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0) },
	['I' - 0x20] = { P(0,0), P(8,0), FONT_UP, P(4,0), P(4,12), FONT_UP, P(0,12), P(8,12) },
	['J' - 0x20] = { P(0,4), P(4,0), P(8,0), P(8,12), FONT_LAST },
	['K' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(8,12), P(0,6), P(6,0), FONT_LAST },
	['L' - 0x20] = { P(8,0), P(0,0), P(0,12), FONT_LAST },
	['M' - 0x20] = { P(0,0), P(0,12), P(4,8), P(8,12), P(8,0), FONT_LAST },
	['N' - 0x20] = { P(0,0), P(0,12), P(8,0), P(8,12), FONT_LAST },
	['O' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,0), P(0,0), FONT_LAST },
	['P' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_LAST },
	['Q' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,4), P(0,0), FONT_UP, P(4,4), P(8,0) },
	['R' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_UP, P(4,5), P(8,0) },
	['S' - 0x20] = { P(0,2), P(2,0), P(8,0), P(8,5), P(0,7), P(0,12), P(6,12), P(8,10) },
	['T' - 0x20] = { P(0,12), P(8,12), FONT_UP, P(4,12), P(4,0), FONT_LAST },
	['U' - 0x20] = { P(0,12), P(0,2), P(4,0), P(8,2), P(8,12), FONT_LAST },
	['V' - 0x20] = { P(0,12), P(4,0), P(8,12), FONT_LAST },
	['W' - 0x20] = { P(0,12), P(2,0), P(4,4), P(6,0), P(8,12), FONT_LAST },
	['X' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(0,12), P(8,0), FONT_LAST },
	['Y' - 0x20] = { P(0,12), P(4,6), P(8,12), FONT_UP, P(4,6), P(4,0), FONT_LAST },
	['Z' - 0x20] = { P(0,12), P(8,12), P(0,0), P(8,0), FONT_UP, P(2,6), P(6,6), FONT_LAST },

	// Special chars from 65 to 96
	[65] = { P(0,0), P(0,12), P(7,12), P(7,0), P(0,0), FONT_LAST },
	[66] = { P(0,0), P(4,12), P(8,0), P(0,0), FONT_LAST },
	[67] = { P(0,6), P(8,6), P(8,7), P(0,7), FONT_LAST },
};

// -----------------------------------------------------------------------------
#pragma mark - Helper

int ret_getbit(BYTE b, BYTE number) {
    return (b >> number) & 1;
}

// -----------------------------------------------------------------------------
#pragma mark - Dirty bit mechanism

BYTE ret_rend_get_dirty_bit() {
    return ret_screen_main.dirty_bit;
}

void ret_rend_enable_dirty_bit(void) {
    ret_screen_main.dirty_bit = 1;
}

void ret_rend_disable_dirty_bit(void) {
    ret_screen_main.dirty_bit = 0;
}

// -----------------------------------------------------------------------------
#pragma mark - Pixel drawing

void ret_rend_set_pixel_internal(int x, int y, int r, int g, int b) {
    int width = ret_screen_main.width;
    int height = ret_screen_main.height;

    // Is the pixel actually visible?
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int offset = (x + ((height - 1) - y) * width) * 4;
        
        // Use this to make the origin top-left instead of bottom-right.
        offset = (x + y * width) * 4;
        
        ret_screen_main.data[offset] = r;
        ret_screen_main.data[offset+1] = g;
        ret_screen_main.data[offset+2] = b;
        ret_screen_main.data[offset+3] = 255;
    }

    ret_rend_enable_dirty_bit();
}

int ret_rend_get_pixel_internal(int x, int y) {
    int width = ret_screen_main.width;
    int height = ret_screen_main.height;

    // Is the pixel actually visible?
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int offset = (x + ((height - 1) - y) * width) * 4;
        
        // Use this to make the origin top-left instead of bottom-right.
        offset = (x + y * width) * 4;
        
        BYTE r = ret_screen_main.data[offset];
        BYTE g = ret_screen_main.data[offset+1];
        BYTE b = ret_screen_main.data[offset+2];
		int color = r + g + b;
		
		return color;
    }
	else {
		return 0;
	}
}

void ret_rend_set_pixel(int x, int y, BYTE paletteColor, BYTE brightness) {
    ret_color fg = RETPaletteGetColorWithBrightness(paletteColor, brightness);
    ret_rend_set_pixel_internal(x, y, fg.r, fg.g, fg.b);
}

void ret_rend_draw_line(int x1, int y1, int x2, int y2, BYTE paletteColor, BYTE brightness)  {
    int dx = x2 - x1;
    int dy = y2 - y1;

    // calculate steps required for generating pixels
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    
    // calculate increment in x & y for each steps
    float xInc = dx / (float) steps;
    float yInc = dy / (float) steps;
    
    float x = x1;
    float y = y1;

	BOOL draw = TRUE;
	int count = 0;
	
    for (int i = 0; i <= steps; i++) {
		if (ret_screen_main.mode_dotted > 0) {
			if (draw) {
				ret_rend_set_pixel (x, y, paletteColor, brightness);
			}
			
			count++;

			if (count == ret_screen_main.mode_dotted) {
				count = 0;
				draw = !draw;
			}
		}
		else {
			ret_rend_set_pixel (x, y, paletteColor, brightness);
		}
		
        x += xInc;
        y += yInc;
    }
}

// -----------------------------------------------------------------------------
#pragma mark - Character vector drawing (internal)

int ret_rend_cursorx_int = 0;
int ret_rend_cursory_int = 0;

void ret_rend_moveto(int x, int y) {
    ret_rend_cursorx_int = x;
    ret_rend_cursory_int = y;
}

void ret_rend_moveby(int dx, int dy) {
    ret_rend_cursorx_int += dx;
    ret_rend_cursory_int += dy;
}

void ret_rend_lineby(int dx, int dy) {
    ret_rend_draw_line(ret_rend_cursorx_int, ret_rend_cursory_int, ret_rend_cursorx_int+dx, ret_rend_cursory_int+dy, ret_screen_main.fg_color, ret_screen_main.brightness);
    ret_rend_moveby(dx, dy);
}

void ret_rend_vec_drawchar(char ch) {
    const BYTE* p = ret_vecfont[ch-0x20];
    BYTE bright = 0;
    BYTE x = 0;
    BYTE y = 0;
    BYTE i;

    for (i=0; i<8; i++) {
        BYTE b = *p++;
        
        if (b == FONT_LAST)
            break; // last move
        else if (b == FONT_UP)
            bright = 0; // pen up
        else {
            BYTE x2 = b>>4;
            BYTE y2 = b&15;

            if (bright == 0)
                ret_rend_moveby((char)(x2-x), (char)-(y2-y));
            else
                ret_rend_lineby((char)(x2-x), (char)-(y2-y));

            bright = 4;
            x = x2;
            y = y2;
        }
    }
}

// -----------------------------------------------------------------------------
#pragma mark - Text drawing

void ret_rend_draw_char(int x, int y, const unsigned char ch, int invert, BYTE paletteColor) {
    int w = RET_FONT_WIDTH;
    int h = RET_FONT_HEIGHT;
    int offset = (int)ch * 8;

	if (ch > 164) {
		offset = 0;
	}
	
    BYTE* dest = ret_screen_main.data + y * (ret_screen_main.width*4) + (x*4);
    
    ret_color bg = RETPaletteGetColor(ret_screen_main.bg_color);
    ret_color fg = RETPaletteGetColorWithBrightness(paletteColor, ret_screen_main.brightness);

	int xStart = x;
    for (int iy = 0; iy < h; iy++) {
        BYTE line = ret_font_speccy[offset+iy];

        for (int ix = w-1; ix >= 0; ix--) {
			if (x >= 0 && x < ret_screen_main.width && y >= 0 && y < ret_screen_main.height) {
                if (ch == ' ' && invert) {
                    // Draw pixel
                    *dest = fg.r; dest++;
                    *dest = fg.g; dest++;
                    *dest = fg.b; dest++;
                    *dest = 0xff; dest++;
                }
				else if (ret_getbit(line, ix) == !invert) {
					// Draw pixel
					*dest = fg.r; dest++;
					*dest = fg.g; dest++;
					*dest = fg.b; dest++;
					*dest = 0xff; dest++;
				}
				else {
					// Erase pixel
					*dest = bg.r; dest++;
					*dest = bg.g; dest++;
					*dest = bg.b; dest++;
					*dest = 0xff; dest++;
				}
			}
			
			// Next pixel
			x++;
        }
        
        // Next line
        y++;
		x = xStart;
		
        dest = ret_screen_main.data + y * (ret_screen_main.width*4) + (x*4);
    }
    
    ret_rend_enable_dirty_bit();
	
	if (ret_screen_main.direct_render) {
		RETRenderFrame();
	}
}

// -----------------------------------------------------------------------------
#pragma mark - Pixel buffer rendering

void ret_rend_clear_screen() {
    int offset = 0;

    ret_color color = RETPaletteGetColor(ret_screen_main.bg_color);

    for (int i = 0 ; i < ret_screen_main.width * ret_screen_main.height ; ++i) {
        ret_screen_main.data[offset] = color.r;
        ret_screen_main.data[offset+1] = color.g;
        ret_screen_main.data[offset+2] = color.b;
        ret_screen_main.data[offset+3] = 255;
        
        offset += 4;
    }
    
    ret_rend_enable_dirty_bit();
}

void ret_rend_scroll_up(int height) {
    BYTE* source;
    BYTE* dest;

    for (int y = height; y < ret_screen_main.height; y++) {
        source = &ret_screen_main.data[y*ret_screen_main.width*4];
        dest = &ret_screen_main.data[(y-height)*ret_screen_main.width*4];
        
        memcpy(dest, source, ret_screen_main.width*4);
    }

    ret_color color = RETPaletteGetColor(ret_screen_main.bg_color);

    // clear last lines
    int offset = (ret_screen_main.height-height) * ret_screen_main.width*4;

    for (int i = 0; i < height*ret_screen_main.width; ++i) {
        ret_screen_main.data[offset] = color.r;
        ret_screen_main.data[offset+1] = color.g;
        ret_screen_main.data[offset+2] = color.b;
        ret_screen_main.data[offset+3] = 255;
        
        offset += 4;
    }

    ret_rend_enable_dirty_bit();
}

// -----------------------------------------------------------------------------
#pragma mark - Private color functions

ret_color ret_rend_get_bg_color() {
	return RETPaletteGetColor(ret_screen_main.bg_color);
}

ret_color ret_rend_get_border_color() {
	return RETPaletteGetColor(ret_screen_main.border_color);
}

// -----------------------------------------------------------------------------
#pragma mark - Character vector drawing (public)

void RETDrawVString(char* str, int x, int y) {
    int x2 = x;
    
    while (*str != 0) {
        char ch = *str;
    
        ret_rend_moveto(x2, y+RET_FONT_HEIGHT);
        ret_rend_vec_drawchar(ch);
        
        str++;
        x2 += RET_FONT_WIDTH+RET_VFONT_SPACE;
    }
}

// -----------------------------------------------------------------------------
#pragma mark - Public color functions

BYTE RETSetBorderColor(BYTE index) {
    BYTE color = ret_screen_main.border_color;
    ret_screen_main.border_color = index;
    
	RETRenderFrame();
	
    return color;
}

BYTE RETSetBgColor(BYTE index) {
    BYTE color = ret_screen_main.bg_color;
    ret_screen_main.bg_color = index;
    
    return color;
}

BYTE RETGetBgColor() {
	return ret_screen_main.bg_color;
}

BYTE RETSetFgColor(BYTE index) {
    BYTE color = ret_screen_main.fg_color;
    ret_screen_main.fg_color = index;
    
    return color;
}

BYTE RETGetFgColor() {
	return ret_screen_main.fg_color;
}

BYTE RETSetBrightness(BYTE index) {
    BYTE brightness = ret_screen_main.brightness;
	ret_screen_main.brightness = index;
    
    return brightness;
}

BYTE RETGetBrightness() {
	return ret_screen_main.brightness;
}

BYTE RETSetDotMode(BYTE mode) {
    BYTE old = ret_screen_main.mode_dotted;
	ret_screen_main.mode_dotted = mode;
    
    return old;
}

// -----------------------------------------------------------------------------
#pragma mark - Public drawing functions

void RETDrawPixel(int x, int y) {
    ret_rend_set_pixel(RET_PIXEL_CENTER_X+x, RET_PIXEL_CENTER_Y+y, ret_screen_main.fg_color, ret_screen_main.brightness);

    ret_screen_main.cursorx = RET_PIXEL_CENTER_X+x;
    ret_screen_main.cursory = RET_PIXEL_CENTER_Y+y;
}

void RETDrawLine(int x1, int y1, int x2, int y2) {
    ret_rend_draw_line(RET_PIXEL_CENTER_X+x1, RET_PIXEL_CENTER_Y+y1, RET_PIXEL_CENTER_X+x2, RET_PIXEL_CENTER_Y+y2, ret_screen_main.fg_color, ret_screen_main.brightness);

    ret_screen_main.cursorx = RET_PIXEL_CENTER_X+x2;
    ret_screen_main.cursory = RET_PIXEL_CENTER_Y+y2;
}

void RETDrawLineTo(int x, int y) {
    ret_rend_draw_line(ret_screen_main.cursorx, ret_screen_main.cursory, x, y, ret_screen_main.fg_color, ret_screen_main.brightness);
    
    ret_screen_main.cursorx = x;
    ret_screen_main.cursory = y;
}

void RETDrawCircle(int xc, int yc, int r) {
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	
	if (!r)
		return;
	
	while (y >= x) {
		// only formulate 1/8 of circle
		RETDrawPixel(xc - x, yc - y); //upper left left
		RETDrawPixel(xc - y, yc - x); //upper upper left
		RETDrawPixel(xc + y, yc - x); //upper upper right
		RETDrawPixel(xc + x, yc - y); //upper right right
		RETDrawPixel(xc - x, yc + y); //lower left left
		RETDrawPixel(xc - y, yc + x); //lower lower left
		RETDrawPixel(xc + y, yc + x); //lower lower right
		RETDrawPixel(xc + x, yc + y); //lower right right
		
		if (p < 0)
			p += 4 * x++ + 6;
		else
			p += 4 * (x++ - y--) + 10;
	}

    ret_screen_main.cursorx = ret_screen_main.cursorx+xc;
    ret_screen_main.cursory = ret_screen_main.cursory+yc;
}

void RETMoveTo(int x, int y) {
    ret_screen_main.cursorx = RET_PIXEL_CENTER_X+x;
    ret_screen_main.cursory = RET_PIXEL_CENTER_Y+y;
}

void RETMoveBy(int x, int y) {
    ret_screen_main.cursorx += x;
    ret_screen_main.cursory += y;
}

void RETRenderFrame(void) {
    platform_render_frame(ret_screen_main.data, ret_screen_main.border_color, 0, ret_screen_main.width, ret_screen_main.height);
}

void RETSetDirectRender(int mode) {
	ret_screen_main.direct_render = mode;
}

// -----------------------------------------------------------------------------
#pragma mark - Dynamic pixel buffer support

void RETSetScreenSize(int width, int height) {
    if (ret_screen_main.data) {
        free(ret_screen_main.data);
    }
    
    ret_screen_main.width = width;
    ret_screen_main.height = height;
    ret_screen_main.data = malloc(ret_screen_main.width*ret_screen_main.height*4);
}

int RETGetScreenWidth(void) {
    return ret_screen_main.width;
}

int RETGetScreenHeight(void) {
    return ret_screen_main.height;
}

// -----------------------------------------------------------------------------
#pragma mark - Pixel buffer initialize

void ret_rend_initialize(int width, int height) {
    ret_screen_main.border_color = RET_COLOR_BG;
    ret_screen_main.bg_color = RET_COLOR_BG;
    ret_screen_main.fg_color = RET_COLOR_FG;
    ret_screen_main.brightness = RET_BRIGHTNESS_NORMAL;
    ret_screen_main.mode_dotted = 0;
    ret_screen_main.dirty_bit = 0;
    ret_screen_main.cursorx = 0;
    ret_screen_main.cursory = 0;
	ret_screen_main.direct_render = 0;
    ret_screen_main.width = width;
    ret_screen_main.height = height;

    // TODO: CHECK malloc
    ret_screen_main.data = malloc(ret_screen_main.width*ret_screen_main.height*4);

    RETMoveTo(0,0);
}
