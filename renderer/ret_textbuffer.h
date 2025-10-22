#ifndef RET_TEXTBUFFER_H
#define RET_TEXTBUFFER_H

#include "rb_types.h"

// Standard color
#define RET_COLOR_BG        RET_COLOR_BLACK
#define RET_COLOR_FG        RET_COLOR_WHITE
#define RET_COLOR_CURSOR    RET_COLOR_GREEN

// Base index to specify color palette as character in a print statament
#define RET_ESCAPE_BASE_INDEX_FOR_COLOR 240
#define RET_ESCAPE_INVERT_ON RET_ESCAPE_BASE_INDEX_FOR_COLOR-2
#define RET_ESCAPE_INVERT_OFF RET_ESCAPE_INVERT_ON+1

void ret_text_initialize(int cols, int rows);
void ret_text_set_immediate_mode(int flag); // 0=text buffering until flush, 1=print directly
void ret_text_flush_buffer(void);
void ret_text_clear_screen(void);
void ret_text_scroll_up(void);

void ret_text_cursor_left(void);
void ret_text_cursor_right(void);
void ret_text_cursor_down(void);
void ret_text_cursor_up(void);
void ret_text_cursor_set(int x, int y);
void ret_text_blink_cursor(void);
void ret_text_cursor_set(int x, int y);
void ret_text_cursor_show(BOOL flag);

const char* ret_text_get_line_buffer(void);
void ret_text_delete_char(void);
BOOL ret_text_print_character(int row, int col, char ch, BYTE color);
char ret_text_get_character(int row, int col);
BYTE ret_text_get_attribute(int row, int col);

void ret_text_print_no_line_break(char* str, int size);

void RETSetInvertMode(BOOL flag);
BOOL RETPrintCharAt(int row, int col, char ch);
void RETPrintAt(int row, int col, char* str);
void RETPrintNewLine(void);
void RETPrintChar(char ch);
void RETPrint(char* str);
void RETPrintLine(char* str);
void RETShowCharacterSet(void);
int RETGetColumns(void);
int RETGetRows(void);

void RETPrintPalette(void);

void RETProcessAsciiKey(int ch);

#endif
