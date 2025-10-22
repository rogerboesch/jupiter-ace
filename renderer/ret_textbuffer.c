
#include "ret_renderer.h"
#include "ret_textbuffer.h"
#include "rb_keycodes.h"
#include "rb_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

int ret_text_cols = 40;                                     // Rows and columns of text buffer
int ret_text_rows = 20;

BYTE *ret_text_screen = 0;		                            // text buffer
BYTE *ret_text_color = 0;		                            // text color buffer

int ret_text_immediate_print = 1;							// 0=needs a flush to print
BOOL ret_text_cursor_visible = TRUE;						// Cursor visible
BOOL ret_invert_mode = FALSE;                               // Draw characer inverted

int ret_cursor_row = 0;										// Cursor position
int ret_cursor_col = 0;
int cursor_blink = 0;										// Cursor blink state, 0=Off
int ret_cursor_cycles = 0;									// Cursor cycle count

// --- Text buffer rendering ---------------------------------------------------
#pragma mark - Text buffer rendering

void ret_text_set_immediate_mode(int flag) {
	// 0=text buffering until flush, 1=direct print
	ret_text_immediate_print = flag;
}

void ret_text_flush_buffer() {
	for (int row = 0; row < ret_text_rows; row++) {
		for (int col = 0; col < ret_text_cols; col++) {
			char ch = ret_text_screen[row*ret_text_cols+col];
			BYTE paletteColor = ret_text_color[row*ret_text_cols+col];

			int x = col * RET_FONT_WIDTH;
			int y = row * RET_FONT_HEIGHT;

			ret_rend_draw_char(x, y, ch, 0, paletteColor);
		}
	}
}

void ret_text_fast_clear_screen() {
	memset(ret_text_screen, 0, ret_text_cols*ret_text_rows);
}

void ret_text_clear_screen() {
	ret_cursor_row = 0;
	ret_cursor_col = 0;

	ret_text_fast_clear_screen();
    ret_text_flush_buffer();
}

BOOL ret_text_print_character(int row, int col, char ch, BYTE color) {
    int width = ret_text_cols;
    int height = ret_text_cols;
        
    BYTE code = (BYTE)ch;
    if (code >= RET_ESCAPE_BASE_INDEX_FOR_COLOR && code <= RET_ESCAPE_BASE_INDEX_FOR_COLOR + 15) {
        // Color escape
        int color = (int)code - RET_ESCAPE_BASE_INDEX_FOR_COLOR;
        RETSetFgColor(color);
        
        return FALSE;
    }
    
    if (code == RET_ESCAPE_INVERT_ON) {
        RETSetInvertMode(TRUE);
        
        return FALSE;
    }

    if (code == RET_ESCAPE_INVERT_OFF) {
        RETSetInvertMode(FALSE);
        
        return FALSE;
    }

    // Is the pixel actually visible?
    if (col >= 0 && col < width && row >= 0 && row < height) {
        int offset = row * ret_text_cols + col;
        
        ret_text_screen[offset] = ch;
        ret_text_color[offset] = color;
        
        if (ret_text_immediate_print == 1) {
            int x = col * RET_FONT_WIDTH;
            int y = row * RET_FONT_HEIGHT;

            if (ret_invert_mode) {
                ret_rend_draw_char(x, y, ch, TRUE, color);
            }
            else {
                ret_rend_draw_char(x, y, ch, FALSE, color);
            }
        }
    }
	
	return TRUE;
}

char ret_text_get_character(int row, int col) {
    int offset = row * ret_text_cols + col;
    return ret_text_screen[offset];
}

BOOL ret_text_print_character_internal(int row, int col, char ch) {
    return ret_text_print_character(row, col, ch, RETGetFgColor());
}

void ret_text_scroll_up() {
	BYTE* source;
	BYTE* dest;
	
	for (int row = 1; row < ret_text_rows; row++) {
		source = &ret_text_screen[row*ret_text_cols];
		dest = &ret_text_screen[(row-1)*ret_text_cols];
		
		memcpy(dest, source, ret_text_cols);
	}

	ret_rend_scroll_up(RET_FONT_HEIGHT);

	// Clear last line
	dest = &ret_text_screen[(ret_text_rows-1)*ret_text_cols];
	memset(dest, '\0', ret_text_cols);
}

// --- Color buffer handling ---------------------------------------------------

BYTE ret_text_get_attribute(int row, int col) {
    int offset = row * ret_text_cols + col;
    return ret_text_color[offset];
}

// --- Cursor handling ---------------------------------------------------------

void ret_text_cursor_show(BOOL flag) {
    ret_text_cursor_visible = flag;
}

void ret_text_draw_cursor() {
    if (!ret_text_cursor_visible) {
        return;
    }
    
	int x = ret_cursor_col * RET_FONT_WIDTH;
	int y = ret_cursor_row * RET_FONT_HEIGHT;

    char ch = ret_text_screen[ret_cursor_row*ret_text_cols+ret_cursor_col];

    ret_rend_draw_char(x, y, ch, 1, RET_COLOR_CURSOR);
}

void ret_text_reset_character() {
	int x = ret_cursor_col * RET_FONT_WIDTH;
	int y = ret_cursor_row * RET_FONT_HEIGHT;

    char ch = ret_text_screen[ret_cursor_row*ret_text_cols+ret_cursor_col];
    BYTE paletteColor = ret_text_color[ret_cursor_row*ret_text_cols+ret_cursor_col];

    ret_rend_draw_char(x, y, ch, 0, paletteColor);
}

void ret_text_cursor_left() {
	ret_text_reset_character();
	
	ret_cursor_col--;
	
	if (ret_cursor_col < 0) {
		ret_cursor_row--;
		ret_cursor_col = ret_text_cols-1;
	}
	
	if (ret_cursor_row < 0) {
		ret_cursor_row = 0;
		ret_cursor_col = 0;
	}

	ret_text_draw_cursor();
}

void ret_text_cursor_right() {
	ret_text_reset_character();

	ret_cursor_col++;
	
	if (ret_cursor_col > ret_text_cols-1) {
		ret_cursor_row++;
		ret_cursor_col = 0;
	}

	if (ret_cursor_row > ret_text_rows-1) {
		ret_text_scroll_up();
		
		ret_cursor_row = ret_text_rows-1;
	}
	
	ret_text_draw_cursor();
}

void ret_text_cursor_up() {
	ret_text_reset_character();

	ret_cursor_row--;
		
	if (ret_cursor_row < 0) {
		ret_cursor_row = 0;
	}

	ret_text_draw_cursor();
}

void ret_text_cursor_down() {
	ret_text_reset_character();

	ret_cursor_row++;
	
	if (ret_cursor_row > ret_text_rows-1) {
		ret_text_scroll_up();

		ret_cursor_row = ret_text_rows-1;
	}
	
	ret_text_draw_cursor();
}

void ret_text_cursor_set(int x, int y) {
    ret_text_reset_character();

    ret_cursor_col = x;
    ret_cursor_row = y;
    
    ret_text_draw_cursor();
}

void ret_text_new_line() {
	ret_text_reset_character();

	ret_cursor_row++;
	ret_cursor_col = 0;
	
	if (ret_cursor_row >= ret_text_rows) {
		// Scroll screen up
		ret_text_scroll_up();
		
		ret_cursor_row = ret_text_rows-1;
	}
	
	ret_text_draw_cursor();
}

void ret_text_blink_cursor() {
	ret_cursor_cycles++;
	if (ret_cursor_cycles < 25)
		return;
	
	ret_cursor_cycles = 0;
	
	if (cursor_blink == 0) {
		ret_text_draw_cursor();
	}
	else {
		ret_text_reset_character();
	}
	
	cursor_blink = !cursor_blink;
}

void ret_text_delete_char(void) {
	ret_text_cursor_left();
	
	// Remove character
	ret_text_screen[ret_cursor_row*ret_cursor_col+ret_cursor_col] = '\0';
}

// --- Private text functions --------------------------------------------------

void ret_text_initialize(int cols, int rows) {
    ret_rend_initialize(cols*RET_FONT_WIDTH, rows*RET_FONT_HEIGHT);
	ret_cursor_row = 0;
	ret_cursor_col = 0;
	cursor_blink = 0;
	ret_cursor_cycles = 0;
    ret_text_rows = rows;
    ret_text_cols = cols;

    // TODO: CHECK malloc
    if (ret_text_screen) {
        free(ret_text_screen);
    }
    ret_text_screen = malloc(ret_text_rows*ret_text_cols);

    if (ret_text_color) {
        free(ret_text_color);
    }
    ret_text_color = malloc(ret_text_rows*ret_text_cols);

	ret_text_fast_clear_screen();
	
	ret_text_draw_cursor();
}

void ret_text_print_no_line_break(char* str, int size) {
    int count = 0;
    while (*str != 0) {
        char ch = *str;
    
        RETPrintCharAt(ret_cursor_row, ret_cursor_col, ch);
        
        ret_cursor_col++;
        if (ret_cursor_col >= RETGetColumns()) {
            ret_cursor_col = RETGetColumns()-1;
        }
        
        ret_text_draw_cursor();

        str++;
        count++;
        
        if (count == size) {
            return;
        }
    }
}

// --- Public text functions ---------------------------------------------------

int RETGetColumns(void) {
    return ret_text_cols;
}

int RETGetRows(void) {
    return ret_text_rows;
}

BOOL RETPrintCharAt(int row, int col, char ch) {
	return ret_text_print_character_internal(row, col, ch);
}

void RETPrintAt(int row, int col, char* str) {
	while (*str != 0) {
		char ch = *str;
	
		RETPrintCharAt(row, col, ch);
		col++;
		
		str++;
	}
}

void RETPrintChar(char ch) {
    if (ch == '\n') {
        ret_text_new_line();
        return;
    }
    
	if (!RETPrintCharAt(ret_cursor_row, ret_cursor_col, ch)) {
		return;
	}
	
	ret_cursor_col++;
	if (ret_cursor_col >= RETGetColumns()) {
		ret_cursor_row++;
		ret_cursor_col = 0;
	}
	
	ret_text_draw_cursor();
}

void RETPrint(char* str) {
	while (*str != 0) {
		char ch = *str;
	
		RETPrintChar(ch);
		
		str++;
	}
}

void RETPrintLine(char* str) {
	RETPrint(str);	
	ret_text_new_line();
}

void RETPrintNewLine() {
    ret_text_new_line();
}

void RETPrintPalette() {
	int fg = RETGetFgColor();

	// Print characters
	for (int i = 0; i < 128; i++) {
		RETSetFgColor(i);
		RETPrintChar((char)233);
	}
	
	RETPrintLine("");
	
	RETSetFgColor(fg);
}

void RETShowCharacterSet() {
    for (int i = 0; i < 256; i++) {
        RETPrintChar(i);
    }
}

void RETSetInvertMode(BOOL flag) {
    ret_invert_mode = flag;
}

// --- Screen editor support ---------------------------------------------------

void RETProcessAsciiKey(int ch) {
	// Check for printable ascii codes
	if (ch == KEY_ENTER) {
		ret_text_new_line();
	}
    else if (ch == KEY_BACKSPACE) {
        // Backspace
        ret_text_delete_char();
    }
    else if (ch == KEY_LEFT) {
        ret_text_cursor_left();
    }
    else if (ch == KEY_RIGHT) {
        ret_text_cursor_right();
    }
    else if (ch == KEY_UP) {
        ret_text_cursor_up();
    }
    else if (ch == KEY_DOWN) {
        ret_text_cursor_down();
    }
	else if (ch >= 32) {
        ch = platform_uppercase(ch);
		RETPrintChar(ch);
	}
}
