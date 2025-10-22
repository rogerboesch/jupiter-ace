
#ifndef ret_texteditor_h
#define ret_texteditor_h

#include "rb_types.h"

BOOL ret_c_editor_is_running(void);

void ret_c_editor_start(char *name, char *extension, char *path);
void ret_c_editor_frame(void);
void ret_c_editor_stop(void);

void ret_c_editor_load(char *name, char *extension, char *path);
int ret_c_editor_save(void);
void ret_c_editor_change_size(int cols, int rows);

#endif 
