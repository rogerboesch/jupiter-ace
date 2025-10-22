
#include "ret_font.h"
#include "ret_renderer.h"
#include "ret_textbuffer.h"
#include "ret_texteditor.h"
#include "rb_keycodes.h"

#include <stdio.h>

extern int platform_get_key(void);
extern ssize_t platform_read_line(char **restrict lineptr, size_t *restrict n, FILE *restrict stream);

const char* temporaryPath(void);

#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#define HL_NORMAL 1
#define HL_NONPRINT 2
#define HL_COMMENT 3
#define HL_MLCOMMENT 4
#define HL_KEYWORD1 5
#define HL_KEYWORD2 6
#define HL_STRING 7
#define HL_NUMBER 8
#define HL_MATCH 9
#define HL_KEYWORD3 10

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

#define EDIT_QUERY_LEN 256

#define FIND_RESTORE_HL do { \
    if (saved_hl) { \
        memcpy(E.row[saved_hl_line].hl,saved_hl, E.row[saved_hl_line].rsize); \
        saved_hl = NULL; \
    } \
} while (0)

enum editorLanguage {
	LANGUAGE_UNKNOWN = 0,
    LANGUAGE_ASM = 1,
    LANGUAGE_C = 2,
    LANGUAGE_BASIC = 3
};

typedef struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[2];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
	int language;
} editorSyntax;

// This structure represents a single line of the file we are editing.
typedef struct editorRow {
    int idx;            // Row index in the file, zero-based.
    int size;           //  Size of the row, excluding the null term.
    int rsize;          //  Size of the rendered row.
    char *chars;        //  Row content.
    char *render;       //  Row content "rendered" for screen (for TABs).
    unsigned char *hl;  //  Syntax highlight type for each character in render.
    int hl_oc;          //  Row had open comment at end in last syntax highlight check.
} editorRow;

typedef struct editorConfig {
    int cx,cy;  				// Cursor x and y position in characters
    int rowoff;     			// Offset of row displayed
    int coloff;    				// Offset of column displayed
    int screenrows; 			// Number of rows that we can show
    int screencols;				// Number of cols that we can show
    int numrows;   				// Number of rows
    int rawmode;   				// Is terminal raw mode enabled?
    editorRow *row;    			// Rows
    int dirty;      			// File modified but not saved
    char *filename; 			// Currently open filename
	char *path; 				// Currently open filename path
    editorSyntax *syntax;    	// Current syntax highlight, or NULL

    char statusmsg[80];
    time_t statusmsg_time;

    int spaces_for_tab;
	BOOL running;
} editorConfig;

static editorConfig E;

void ret_c_editor_set_status_message(const char *fmt, ...);
void ret_c_editor_wait_for_keypress(void);

// C Language
char *C_HL_extensions[] = {"c", "C", NULL};
char *C_HL_keywords[] = {
    // Keywords (Statements)
    "switch","if","while","for","break","continue","return","else",
    "struct","union","typedef","static","enum",
    
    // Types "!" indicates a different set of keywords (types)
    "int!","long!","double!","float!","char!","unsigned!","signed!",
    "void!",
    
    // Types "?" indicates a different set of keywords (functions)
    "vdot?","vlineto?","vmoveto?","vmoveby?","vprintat?",
	"vsetdotmode?","vsetcolor?",
	"vposition?","vcls?",
	"vbegin_frame?","vend_frame?","vgraphics?","vgraphics_frame?",NULL
};

// Basic Language
char *BAS_HL_extensions[] = {"bas", "BAS", NULL};
char *BAS_HL_keywords[] = {
    // Keywords
    "AT",
    "BEEP","BIN","BORDER","BRIGHT",
    "CAT","CIRCLE","CLEAR","CLOSE","CLS","CONTINUE","COPY",
    "DATA","DEF","DIM","DUMP","DRAW",
    "EDIT","ERASE",
    "FLASH","FN","FORMAT","FOR",
    "GOSUB","GOTO",
    "IF","INK","INPUT","INVERSE",
    "LET","LINE","LIST","LLIST",
    "LOAD","LPRINT",
    "MERGE","MOVE",
    "NEW","NEXT",
    "OPEN","OUT","OVER",
    "PAPER","PAUSE","PLOT","POINT","POKE","PRINT",
    "RANDOMIZE","READ","REM","RESTORE","RETURN","RUN",
    "SAVE","STEP","STOP",
    "TAB","THEN","TO",
    "USR",
    "VERIFY",
    
    // Types "!" indicates a different set of keywords (functions)
    "ABS!","ACS!","AND!","ASN!","ATN!","ATTR!",
    "CHR$!","CODE!","COS!",
    "EXP!",
    "IN!","INKEY$!","INT!",
    "LEN!","LN!",
    "MEM!",
    "NOT!",
    "OR!",
    "PEEK!","PI!",
    "RND!",
    "SCREEN$!","SGN!","SIN!","SQR!","STR$!",
    "TAN!",
    "VAL!","VAL$!",

    // Types "?" indicates a different set of keywords (not used yet)
    NULL
};

// 6809 Assembler Language
char *ASM_HL_extensions[] = {"asm", "ASM", NULL};
char *ASM_HL_keywords[] = {
    // Keywords
    "ORG","CODE","FCB","FCC","BSS","DB","DW",
    "LDA","LDB","LDD","LDS","LDU","LDX","LDY",
    "STA","STB","STD","STS","STU","STX","STY",
    "INCA","INCB","INCD","INCS","INCU","INCX","INCY",
    "DECA","DECB","DECD","DECS","DECU","DECX","DECY",
    "EQU","TFR","RTS", "FDB",
    "BCC","BCS","BEQ","BGE","BGT","BHI","BHS","BLE","BLO","BLS","BLT","BMI",
    "BNE","BPL","BRA","BRN","BSR","BVC","BVS","JMP","JSR",
    "INCLUDE",


    // Types "!" indicates a different set of keywords (BIOS functions)
    "Vec_Snd_Shadow!","Vec_Btn_State!","Vec_Prev_Btns!","Vec_Buttons!","Vec_Button_1_1!","Vec_Button_1_2!","Vec_Button_1_3!","Vec_Button_1_4!",
     "Vec_Button_2_1!","Vec_Button_2_2!","Vec_Button_2_3!","Vec_Button_2_4!","Vec_Joy_Resltn!","Vec_Joy_1_X!","Vec_Joy_1_Y!","Vec_Joy_2_X!","Vec_Joy_2_Y!",
     "Vec_Joy_Mux!","Vec_Joy_Mux_1_X!","Vec_Joy_Mux_1_Y!","Vec_Joy_Mux_2_X!","Vec_Joy_Mux_2_Y!","Vec_Misc_Count!","Vec_0Ref_Enable!","Vec_Loop_Count!",
     "Vec_Brightness!","Vec_Dot_Dwell!","Vec_Pattern!","Vec_Text_HW!","Vec_Text_Height!","Vec_Text_Width!","Vec_Str_Ptr!","Vec_Counters!","Vec_Counter_1!","Vec_Counter_2"
     "Vec_Counter_3!","Vec_Counter_4!","Vec_Counter_5!","Vec_Counter_6!","Vec_RiseRun_Tmp!","Vec_Angle!", "Vec_Run_Index!","Vec_Rise_Index!","Vec_RiseRun_Len!",
     "Vec_Rfrsh!","Vec_Rfrsh_lo!","Vec_Rfrsh_hi!","Vec_Music_Work!","Vec_Music_Wk_A!","Vec_Music_Wk_7!","Vec_Music_Wk_6!","Vec_Music_Wk_5!","Vec_Music_Wk_1!","Vec_Freq_Table!",
     "Vec_Max_Players!","Vec_Max_Games!","Vec_ADSR_Table!","Vec_Twang_Table!","Vec_Music_Ptr!","Vec_Expl_ChanA!","Vec_Expl_Chans!","Vec_Music_Chan!","Vec_Music_Flag!",
     "Vec_Duration!","Vec_Music_Twang!","Vec_Expl_1!","Vec_Expl_2!","Vec_Expl_3!","Vec_Expl_4!","Vec_Expl_Chan!","Vec_Expl_ChanB!","Vec_ADSR_Timers!",
     "Vec_Music_Freq!","Vec_Expl_Flag!","Vec_Expl_Timer!","Vec_Num_Players!","Vec_Num_Game!","Vec_Seed_Ptr!","Vec_Random_Seed!","Vec_Default_Stk!",
     "Vec_High_Score!","Vec_SWI3_Vector!","Vec_SWI2_Vector!","Vec_FIRQ_Vector!","Vec_IRQ_Vector!","Vec_SWI_Vector!","Vec_NMI_Vector!","Vec_Cold_Flag!",
     "VIA_port_b!","VIA_port_a!","VIA_DDR_b!","VIA_DDR_a!","VIA_t1_cnt_lo!","VIA_t1_cnt_hi!","VIA_t1_lch_lo!","VIA_t1_lch_hi!","VIA_t2_lo!","VIA_t2_hi!",
     "VIA_shift_reg!","VIA_aux_cntl!","VIA_cntl !","VIA_int_flags!","VIA_int_enable!","VIA_port_a_nohs!","Cold_Start!","Warm_Start!","Init_VIA !","Init_OS_RAM!","Init_OS!",
     "Wait_Recal!","Set_Refresh!","DP_to_D0!","DP_to_C8!","Read_Btns_Mask!","Read_Btns!","Joy_Analog!","Joy_Digital!","Sound_Byte!","Sound_Byte_x!","Sound_Byte_raw!","Clear_Sound!",
     "Sound_Bytes!","Sound_Bytes_x!","Do_Sound !","Do_Sound_x!","Intensity_1F!","Intensity_3F!","Intensity_5F!","Intensity_7F!","Intensity_a!",
     "Dot_ix_b!","Dot_ix!","Dot_d!","Dot_here!","Dot_List!","Recalibrate!","Moveto_x_7F!","Moveto_d_7F!","Moveto_ix_FF!","Moveto_ix_7F!","Moveto_ix_b!",
     "Moveto_ix!","Moveto_d!","Reset0Ref_D0!","Check0Ref!","Reset0Ref!","Reset_Pen!","Reset0Int!","Print_Str_hwyx!","Print_Str_yx!", "Print_Str_d!",
     "Print_List_hw!","Print_List!","Print_List_chk!","Print_Ships_x!","Print_Ships!","Mov_Draw_VLc_a!","Mov_Draw_VL_b!","Mov_Draw_VLcs!","Mov_Draw_VL_ab!","Mov_Draw_VL_a!",
     "Mov_Draw_VL!","Mov_Draw_VL_d!","Draw_VLc!","Draw_VL_b!","Draw_VLcs!","Draw_VL_ab!","Draw_VL_a!","Draw_VL!","Draw_Line_d!","Draw_VLp_FF!","Draw_VLp_7F!",
     "Draw_VLp_scale!","Draw_VLp_b!","Draw_VLp!","Draw_Pat_VL_a!","Draw_Pat_VL!","Draw_Pat_VL_d!","Draw_VL_mode!","Print_Str!","Random_3!","Random!",
     "Init_Music_Buf!","Clear_x_b!","Clear_C8_RAM!","Clear_x_256!","Clear_x_d!","Clear_x_b_80!","Clear_x_b_a!","Dec_3_Counters!","Dec_6_Counters!","Dec_Counters!",
     "Delay_3!","Delay_2!","Delay_1!","Delay_0!","Delay_b!","Delay_RTS!","Bitmask_a!","Abs_a_b!","Abs_b!","Rise_Run_Angle!","Get_Rise_Idx!",
     "Get_Run_Idx!","Get_Rise_Run!","Rise_Run_X!","Rise_Run_Y!","Rise_Run_Len!","Rot_VL_ab!","Rot_VL!","Rot_VL_Mode!","Rot_VL_M_dft!","Xform_Run_a!",
     "form_Run!","Xform_Rise_a!","Xform_Rise!","Move_Mem_a_1!","Move_Mem_a!","Init_Music_chk!","Init_Music!","Init_Music_x!","Select_Game!","Clear_Score!",
     "Add_Score_a!", "Add_Score_d!","Strip_Zeros!","Compare_Score!","New_High_Score!","Obj_Will_Hit_u!","Obj_Will_Hit!","Obj_Hit!","Explosion_Snd!","Draw_Grid_VL!",
     "music1!","music2!","music3!","music4!","music5!","music6!","music7!","music8!","music9!","musica!","musicb!","musicc!","musicd!","Char_Table!","Char_Table_End!",

    // Types "?" indicates a different set of keywords (not used yet)
    NULL
};

struct editorSyntax HLDB[] = {
    {
        // ASM
        ASM_HL_extensions,
        ASM_HL_keywords,
        ";","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS,
        LANGUAGE_ASM
    },
    {
        // C
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS,
        LANGUAGE_C
    },
    {
        // Basic
        BAS_HL_extensions,
        BAS_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS,
		LANGUAGE_BASIC
    }
};

#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

// -----------------------------------------------------------------------------
#pragma mark - Helper methods

int ret_c_editor_read_key() {
	int c = 0;

    while (1) {
		c = platform_get_key();
		if (c != 0)
			return c;
	}
}

int ret_c_editor_is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];",c) != NULL;
}

// -----------------------------------------------------------------------------
#pragma mark - Syntax highlighting

int ret_c_editor_row_has_open_comment(editorRow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT && (row->rsize < 2 || (row->render[row->rsize-2] != '*' ||
        row->render[row->rsize-1] != '/'))) {
        return 1;
    }
    
    return 0;
}

void ret_c_editor_update_syntax(editorRow *row) {
    row->hl = realloc(row->hl,row->rsize);
    memset(row->hl,HL_NORMAL,row->rsize);

    if (E.syntax == NULL) {
        // No syntax
        return;
    }

    int i, prev_sep, in_string, in_comment;
    char *p;
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    p = row->render;
    i = 0;

    while (*p && isspace(*p)) {
        p++;
        i++;
    }
    
    prev_sep = 1;
    in_string = 0;
    in_comment = 0; // Are we inside multi-line comment?

    // If the previous line has an open comment, this line starts with an open comment state.
    if (row->idx > 0 && ret_c_editor_row_has_open_comment(&E.row[row->idx-1])) {
        in_comment = 1;
    }

    while (*p) {
        // Handle single line comments
        if (prev_sep && *p == scs[0] && *(p+1) == scs[1]) {
            memset(row->hl+i, HL_COMMENT, row->size-i);
            return;
        }

        // Handle multi line comments
        if (in_comment) {
            row->hl[i] = HL_MLCOMMENT;
            
            if (*p == mce[0] && *(p+1) == mce[1]) {
                row->hl[i+1] = HL_MLCOMMENT;
                p += 2;
				i += 2;
                in_comment = 0;
                prev_sep = 1;
                
                continue;
            }
            else {
                prev_sep = 0;
                p++;
				i++;
                
                continue;
            }
        }
        else if (*p == mcs[0] && *(p+1) == mcs[1]) {
            row->hl[i] = HL_MLCOMMENT;
            row->hl[i+1] = HL_MLCOMMENT;
            p += 2;
			i += 2;
            in_comment = 1;
            prev_sep = 0;
            
            continue;
        }

        // Handle "" and ''
        if (in_string) {
            row->hl[i] = HL_STRING;
            
            if (*p == '\\') {
                row->hl[i+1] = HL_STRING;
                p += 2;
				i += 2;
                prev_sep = 0;
                
                continue;
            }
            
            if (*p == in_string) {
                in_string = 0;
            }
            
            p++;
            i++;
            
            continue;
        }
        else {
            if (*p == '"' || *p == '\'') {
                in_string = *p;
                row->hl[i] = HL_STRING;
                p++;
                i++;
                prev_sep = 0;
                
                continue;
            }
        }

        // Handle non printable chars.
        if (!isprint(*p)) {
            row->hl[i] = HL_NONPRINT;
            p++;
            i++;
            prev_sep = 0;
            
            continue;
        }

        // Handle numbers
        if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) || (*p == '.' && i >0 && row->hl[i-1] == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            p++;
            i++;
            prev_sep = 0;
            
            continue;
        }

        // Handle keywords and lib calls
        if (prev_sep) {
            int j;
            
            for (j = 0; keywords[j]; j++) {
                int klen = (int)strlen(keywords[j]);
                
				int kw2 = keywords[j][klen-1] == '!';
                if (kw2) {
                    klen--;
                }
				
                int kw3 = keywords[j][klen-1] == '?';
                if (kw3) {
                    klen--;
                }

                if (!memcmp(p, keywords[j], klen) && ret_c_editor_is_separator(*(p+klen))) {
                    // Keyword
					if (kw2)
						memset(row->hl+i, HL_KEYWORD2, klen);
					else if (kw3)
						memset(row->hl+i, HL_KEYWORD3, klen);
					else
						memset(row->hl+i, HL_KEYWORD1, klen);

                    p += klen;
                    i += klen;
                    
                    break;
                }
            }
            
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue; // We had a keyword match
            }
        }

        // Normal chars
        prev_sep = ret_c_editor_is_separator(*p);
        p++; i++;
    }

    // Propagate syntax change to the next row if the open commen
    // state changed. This may recursively affect all the following rows in the file.
    int oc = ret_c_editor_row_has_open_comment(row);
    
    if (row->hl_oc != oc && row->idx+1 < E.numrows) {
        ret_c_editor_update_syntax(&E.row[row->idx+1]);
    }
    
    row->hl_oc = oc;
}

int ret_c_editor_syntax_to_color(int hl) {
    switch (hl) {
        case HL_COMMENT:
        case HL_MLCOMMENT:
            return RET_COLOR_YELLOW;
        case HL_KEYWORD1:
            return RET_COLOR_RED;
        case HL_KEYWORD2:
            return RET_COLOR_MAGENTA;
        case HL_KEYWORD3:
            return RET_COLOR_GREEN;
        case HL_STRING:
            return RET_COLOR_CYAN;
        case HL_NUMBER:
            return RET_COLOR_YELLOW;
        case HL_MATCH:
            return RET_COLOR_RED;
			
        default:
            return RET_COLOR_WHITE;
    }
}

void ret_c_editor_select_syntax_highlight(char *filename) {
	for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = HLDB+j;
        unsigned int i = 0;
		
        while (s->filematch[i]) {
            char *p;
            int patlen = (int)strlen(s->filematch[i]);
			
            if ((p = strstr(filename, s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    E.syntax = s;
                    
                    return;
                }
            }
            
            i++;
        }
    }
}

// -----------------------------------------------------------------------------
#pragma mark - Editor rows implementation

// Update the rendered version and the syntax highlight of a row.
void ret_c_editor_update_row(editorRow *row) {
    int tabs = 0, nonprint = 0, j, idx;

    // Create a version of the row we can directly print on the screen.
    // respecting tabs, substituting non printable characters with '?'. */
    free(row->render);
    
    for (j = 0; j < row->size; j++) {
		if (row->chars[j] == KEY_TAB) {
			tabs++;
		}
    }

    row->render = malloc(row->size + tabs*E.spaces_for_tab + nonprint*9 + 1);
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == KEY_TAB) {
            for (int i = 1; i <= E.spaces_for_tab; ++i) {
                row->render[idx++] = ' ';
            }
            
            //while ((idx+1) % E.spaces_for_tab != 0) {
            //    row->render[idx++] = ' ';
            //}
        }
        else {
            row->render[idx++] = row->chars[j];
        }
    }
    
    row->rsize = idx;
    row->render[idx] = '\0';

    // Update the syntax highlighting attributes of the row.
    ret_c_editor_update_syntax(row);
}

// Insert a row at the specified position, shifting the other rows on the bottom if required.
void ret_c_editor_insert_row(int at, char *s, int len) {
    if (at > E.numrows) {
        return;
    }
    
    E.row = realloc(E.row,sizeof(editorRow)*(E.numrows+1));
    if (at != E.numrows) {
        memmove(E.row+at+1, E.row+at, sizeof(E.row[0])*(E.numrows-at));
		
		for (int j = at+1; j <= E.numrows; j++) {
			E.row[j].idx++;
		}
    }
	
    E.row[at].size = len;
    E.row[at].chars = malloc(len+1);
    memcpy(E.row[at].chars, s,len+1);
    E.row[at].hl = NULL;
    E.row[at].hl_oc = 0;
    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    E.row[at].idx = at;
    ret_c_editor_update_row(E.row+at);
    
    E.numrows++;
    E.dirty++;
}

void ret_c_editor_free_row(editorRow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

// Remove the row at the specified position, shifting the remainign on the top
void ret_c_editor_del_row(int at) {
    editorRow *row;

    if (at >= E.numrows) {
        return;
    }
    
    row = E.row+at;
    ret_c_editor_free_row(row);
    memmove(E.row+at, E.row+at+1, sizeof(E.row[0])*(E.numrows-at-1));
    
    for (int j = at; j < E.numrows-1; j++) {
        E.row[j].idx++;
    }
    
    E.numrows--;
    E.dirty++;
}

// Turn the editor rows into a single heap-allocated string.
// Returns the pointer to the heap-allocated string and populate the integer pointed by 'buflen' with the size of the string,
// excluding the final nulterm.
char *ret_c_editor_rows_to_string(int *buflen) {
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    for (j = 0; j < E.numrows; j++) {
        totlen += E.row[j].size+1; // +1 is for "\n" at end of every row
    }
    
    *buflen = totlen;
    totlen++; // Space for nulterm

    p = buf = malloc(totlen);
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    
    *p = '\0';
    return buf;
}

// Insert a character at the specified position in a row, moving the remaining chars on the right if needed.
void ret_c_editor_row_insert_char(editorRow *row, int at, int c) {
    if (at > row->size) {
        int padlen = at-row->size;

        row->chars = realloc(row->chars,row->size+padlen+2);
        memset(row->chars+row->size, ' ', padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    }
    else {
        row->chars = realloc(row->chars,row->size+2);
        memmove(row->chars+at+1, row->chars+at, row->size-at+1);
        row->size++;
    }

    row->chars[at] = c;
    ret_c_editor_update_row(row);
    
    E.dirty++;
}

// Append the string 's' at the end of a row
void ret_c_editor_row_append_string(editorRow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size+len+1);
    memcpy(row->chars+row->size,s,len);
    
    row->size += len;
    row->chars[row->size] = '\0';
    
    ret_c_editor_update_row(row);
    E.dirty++;
}

// Delete the character at offset 'at' from the specified row
void ret_c_editor_row_del_char(editorRow *row, int at) {
    if (row->size <= at) {
        return;
    }

    memmove(row->chars+at, row->chars+at+1, row->size-at);
    ret_c_editor_update_row(row);
    
    row->size--;
    E.dirty++;
}

// Insert the specified char at the current prompt position
void ret_c_editor_insert_char(int c) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    editorRow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    // If the row where the cursor is currently located does not exist in our
    // logical representaion of the file, add enough empty rows as needed.
    if (!row) {
        while (E.numrows <= filerow) {
            ret_c_editor_insert_row(E.numrows, "", 0);
        }
    }
    
    row = &E.row[filerow];
    ret_c_editor_row_insert_char(row, filecol, c);
    
	if (E.cx == E.screencols-1) {
        E.coloff++;
	}
	else {
        E.cx++;
	}
	
    E.dirty++;
}

// Inserting a newline is slightly complex as we have to handle inserting a
// newline in the middle of a line, splitting the line as needed.
void ret_c_editor_insert_newline(void) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    editorRow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) {
        if (filerow == E.numrows) {
            ret_c_editor_insert_row(filerow, "", 0);
            goto fixcursor;
        }
        
        return;
    }
    
    // If the cursor is over the current line size, we want to conceptually
    // think it's just over the last character.
    if (filecol >= row->size) {
        filecol = row->size;
    }
    
    if (filecol == 0) {
        ret_c_editor_insert_row(filerow, "", 0);
    }
    else {
        // We are in the middle of a line. Split it between two rows.
        ret_c_editor_insert_row(filerow+1, row->chars+filecol, row->size-filecol);
        
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        ret_c_editor_update_row(row);
    }
    
fixcursor:
    if (E.cy == E.screenrows-1) {
        E.rowoff++;
    }
    else {
        E.cy++;
    }
    
    E.cx = 0;
    E.coloff = 0;
}

// Delete the char at the current prompt position.
void ret_c_editor_del_char() {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    editorRow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row || (filecol == 0 && filerow == 0)) {
        return;
    }
    
    if (filecol == 0) {
        // Handle the case of column 0, we need to move the current line on the right of the previous one.
        filecol = E.row[filerow-1].size;

        ret_c_editor_row_append_string(&E.row[filerow-1], row->chars,row->size);
        ret_c_editor_del_row(filerow);

        row = NULL;

		if (E.cy == 0) {
            E.rowoff--;
		}
		else {
            E.cy--;
		}
		
        E.cx = filecol;

        if (E.cx >= E.screencols) {
            int shift = (E.screencols-E.cx)+1;
            
            E.cx -= shift;
            E.coloff += shift;
        }
    }
    else {
        ret_c_editor_row_del_char(row, filecol-1);
        
		if (E.cx == 0 && E.coloff) {
            E.coloff--;
		}
		else {
            E.cx--;
		}
    }
    
	if (row) {
        ret_c_editor_update_row(row);
	}
	
    E.dirty++;
}

// -----------------------------------------------------------------------------
#pragma mark - Editor load/save

// Load the specified file in the editor memory
int ret_c_editor_open(char* name, char* extension, char* path) {
    FILE *fp;

	char filename[512];
	sprintf(filename, "%s.%s", name, extension);

    E.dirty = 0;
    free(E.filename);
    E.filename = strdup(filename);
	
    free(E.path);
    E.path = strdup(path);

	char fullPath[512];
	sprintf(fullPath, "%s/%s.%s", path, name, extension);
	
    fp = fopen(fullPath, "r");
    if (!fp) {
        if (errno != ENOENT) {
            ret_c_editor_set_status_message("Error open file: %s", filename);
        }
        
        return 1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
     
    while ((linelen = platform_read_line(&line, &linecap, fp)) != -1) {
        if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
            line[--linelen] = '\0';
        }
        
        ret_c_editor_insert_row(E.numrows, line, (int)linelen);
    }
    
    free(line);
    fclose(fp);
    E.dirty = 0;
    
    return 0;
}

// Save the current file
int ret_c_editor_save(void) {
    int len;
    char *buf = ret_c_editor_rows_to_string(&len);

	char path[256];
	sprintf(path, "%s/%s", E.path, E.filename);

    int fd = open(path, O_RDWR|O_CREAT, 0644);
    if (fd == -1) {
        goto writeerr;
    }
    
    // Use truncate + a single write(2) call in order to make saving
    // a bit safer, under the limits of what we can do in a small editor.
    if (ftruncate(fd, len) == -1) {
        goto writeerr;
    }
    
    if (write(fd, buf, len) != len) {
        goto writeerr;
    }
    
    close(fd);
    free(buf);
    
    E.dirty = 0;
    ret_c_editor_set_status_message("File saved (%d bytes)", len);
    
    return 0;

writeerr:
    free(buf);
    
    if (fd != -1) {
        close(fd);
    }
    
    ret_c_editor_set_status_message("Error save file: [%lu]", errno);
    
    return 1;
}

// -----------------------------------------------------------------------------
#pragma mark - Screen update

void ret_c_editor_draw_cursor(int row, int col, char ch) {
    int x = (col) * RET_FONT_WIDTH;
    int y = (row) * RET_FONT_HEIGHT;

    ret_rend_draw_char(x, y, ch, 1, RET_COLOR_CURSOR);
}

void ret_c_editor_write_to_text_buffer(char* str, int size) {
	ret_text_print_no_line_break(str, size);
}

void ret_c_editor_write_new_line() {
	RETPrintNewLine();
}

// This function writes the whole screen using VT100 escape characters
// starting from the logical state of the editor in the global state 'E'
void ret_c_editor_refresh_screen() {
    int y;
    editorRow *r;

	ret_text_clear_screen();
	ret_text_cursor_set(0, 0);

    for (y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff+y;

        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows/3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "PICO-EDITOR");
                
                int padding = (E.screencols-welcomelen)/2;

                if (padding) {
                    ret_c_editor_write_to_text_buffer("~", 1);
                    padding--;
                }

                while (padding--) {
                    ret_c_editor_write_to_text_buffer(" ", 1);
                }
                
                ret_c_editor_write_to_text_buffer(welcome, welcomelen);
                ret_c_editor_write_new_line();
            }
			else {
                ret_c_editor_write_to_text_buffer("~", 1);
				ret_c_editor_write_new_line();
            }
			
            continue;
        }

        r = &E.row[filerow];

        int len = r->rsize - E.coloff;
        int current_color = -1;
        if (len > 0) {
            if (len > E.screencols) {
                len = E.screencols;
            }
            
            char *c = r->render+E.coloff;
            unsigned char *hl = r->hl+E.coloff;
            int j;
            
            for (j = 0; j < len; j++) {
                if (hl[j] == HL_NONPRINT) {
                    char sym;

					RETSetInvertMode(TRUE);

					if (c[j] <= 26) {
                        sym = '@'+c[j];
					}
					else {
                        sym = '?';
					}
					
                    ret_c_editor_write_to_text_buffer(&sym, 1);

					RETSetInvertMode(FALSE);
                }
                else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        int color = ret_c_editor_syntax_to_color(HL_NORMAL);
						RETSetFgColor(color);
                        current_color = -1;
                    }
                    
                    ret_c_editor_write_to_text_buffer(c+j, 1);
                }
                else {
                    int color = ret_c_editor_syntax_to_color(hl[j]);
                
                    if (color != current_color) {
						RETSetFgColor(color);
                        current_color = color;
                    }
                    
                    ret_c_editor_write_to_text_buffer(c+j, 1);
                }
            }
        }

		ret_c_editor_write_new_line();
        RETSetFgColor(RET_COLOR_WHITE);
    }

	RETSetInvertMode(TRUE);
    RETSetFgColor(RET_COLOR_CURSOR);
    
    // Create a two rows status
    char status[512], rstatus[512]; // Workaround, should also be dynamic
    int len = snprintf(status, sizeof(status), "%d lines %s", E.numrows, E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.rowoff+E.cy+1, E.coloff+E.cx+1);
    
    if (len > E.screencols) {
        len = E.screencols;
    }
    
    ret_c_editor_write_to_text_buffer(status, len);
	
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            ret_c_editor_write_to_text_buffer(rstatus, rlen);
            break;
        }
        else {
            ret_c_editor_write_to_text_buffer(" ", 1);
            len++;
        }
    }

    RETSetFgColor(RET_COLOR_WHITE);

	ret_c_editor_write_new_line();
	RETSetInvertMode(FALSE);

    // Second row depends on E.statusmsg and the status message update time.
	int timeLeft = (int)(time(NULL)-E.statusmsg_time);
    int msglen = (int)strlen(E.statusmsg);
	
	if (msglen > 0 && timeLeft < 5) {
        ret_c_editor_write_to_text_buffer(E.statusmsg, msglen <= E.screencols ? msglen : E.screencols);
	}
	
    // Put cursor at its current position. Note that the horizontal position
    // at which the cursor is displayed may be different compared to 'E.cx' because of TABs.
    int j;
    int cx = 0;
    int filerow = E.rowoff+E.cy;
    editorRow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (row) {
        for (j = E.coloff; j < (E.cx+E.coloff); j++) {
            if (j < row->size && row->chars[j] == KEY_TAB) {
                cx += E.spaces_for_tab-1-((cx)%E.spaces_for_tab);
            }
            
            cx++;
        }
    }

    char c = ret_text_get_character(E.cy, cx);
    ret_c_editor_draw_cursor(E.cy, cx, c);

	RETRenderFrame();
}

// -----------------------------------------------------------------------------
#pragma mark - Screen message

void ret_c_editor_set_status_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(E.statusmsg,sizeof(E.statusmsg),fmt,ap);
    va_end(ap);
	
    E.statusmsg_time = time(NULL);
	
	ret_c_editor_refresh_screen();
}

// -----------------------------------------------------------------------------
#pragma mark - Find mode

void ret_c_editor_find() {
    char query[EDIT_QUERY_LEN+1] = {0};
    int qlen = 0;
    int last_match = -1;
    int find_next = 0;          // if 1 search next, if -1 search prev.
    int saved_hl_line = -1;     // No saved HL
    char *saved_hl = NULL;

    // Save the cursor position in order to restore it later.
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    while (1) {
        ret_c_editor_set_status_message("Search: %s (Use ESC/Arrows/Enter)", query);
        ret_c_editor_refresh_screen();

        int c = ret_c_editor_read_key();
        if (c == KEY_DELETE || c == KEY_BACKSPACE) {
            if (qlen != 0) {
                query[--qlen] = '\0';
            }
            
            last_match = -1;
        }
        else if (c == KEY_ESC || c == KEY_ENTER) {
            if (c == KEY_ESC) {
                E.cx = saved_cx;
				E.cy = saved_cy;
                E.coloff = saved_coloff;
				E.rowoff = saved_rowoff;
            }
            
            FIND_RESTORE_HL;
            ret_c_editor_set_status_message("");
            
            return;
        }
        else if (c == KEY_RIGHT || c == KEY_DOWN) {
            find_next = 1;
        }
        else if (c == KEY_LEFT || c == KEY_UP) {
            find_next = -1;
        }
        else if (isprint(c)) {
            if (qlen < EDIT_QUERY_LEN) {
                query[qlen++] = c;
                query[qlen] = '\0';
                last_match = -1;
            }
        }

        // Search occurrence.
        if (last_match == -1) {
            find_next = 1;
        }
        
        if (find_next) {
            char *match = NULL;
            int match_offset = 0;
            int i, current = last_match;

            for (i = 0; i < E.numrows; i++) {
                current += find_next;
				
				if (current == -1) {
                    current = E.numrows-1;
				}
				else if (current == E.numrows) {
                    current = 0;
				}
                
                match = strstr(E.row[current].render, query);
                if (match) {
                    match_offset = (int)(match-E.row[current].render);
                    break;
                }
            }
            
            find_next = 0;

            // Highlight
            FIND_RESTORE_HL;

            if (match) {
                editorRow *row = &E.row[current];
                last_match = current;
                
                if (row->hl) {
                    saved_hl_line = current;
                    saved_hl = malloc(row->rsize);
                    memcpy(saved_hl, row->hl, row->rsize);
                    memset(row->hl+match_offset, HL_MATCH, qlen);
                }
                
                E.cy = 0;
                E.cx = match_offset;
                E.rowoff = current;
                E.coloff = 0;

                // Scroll horizontally as needed.
                if (E.cx > E.screencols) {
                    int diff = E.cx - E.screencols;
                    E.cx -= diff;
                    E.coloff += diff;
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
#pragma mark - Editor events handling

// Handle cursor position change because arrow keys were pressed.
void ret_c_editor_move_cursor(int key) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    int rowlen;
    editorRow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch(key) {
		case KEY_LEFT:
			if (E.cx == 0) {
				if (E.coloff) {
					E.coloff--;
				}
				else {
					if (filerow > 0) {
						E.cy--;
						E.cx = E.row[filerow-1].size;
						
						if (E.cx > E.screencols-1) {
							E.coloff = E.cx-E.screencols+1;
							E.cx = E.screencols-1;
						}
					}
				}
			}
			else {
				E.cx -= 1;
			}
			break;
		case KEY_RIGHT:
			if (row && filecol < row->size) {
				if (E.cx == E.screencols-1) {
					E.coloff++;
				}
				else {
					E.cx += 1;
				}
			}
			else if (row && filecol == row->size) {
				E.cx = 0;
				E.coloff = 0;
				
				if (E.cy == E.screenrows-1) {
					E.rowoff++;
				}
				else {
					E.cy += 1;
				}
			}
			break;
		case KEY_UP:
			if (E.cy == 0) {
				if (E.rowoff) {
					E.rowoff--;
				}
			}
			else {
				E.cy -= 1;
			}
			break;
		case KEY_DOWN:
			if (filerow < E.numrows) {
				if (E.cy == E.screenrows-1) {
					E.rowoff++;
				}
				else {
					E.cy += 1;
				}
			}
			break;
    }
    
    // Fix cx if the current line has not enough chars.
    filerow = E.rowoff+E.cy;
    filecol = E.coloff+E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    
    if (filecol > rowlen) {
        E.cx -= filecol-rowlen;
        
        if (E.cx < 0) {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

void ret_c_editor_process_keypress() {
	int c = ret_c_editor_read_key();
    
    switch (c) {
		case KEY_ENTER:
			ret_c_editor_insert_newline();
			break;
		case KEY_F2:
			ret_c_editor_save();
			break;
		case KEY_F1:
			ret_c_editor_find();
			break;
		case KEY_BACKSPACE:
		case KEY_DELETE:
			ret_c_editor_del_char();
			break;
        case KEY_HOME:
            E.cx = 0;
            E.cy = 0;
            break;
		case KEY_PAGE_UP:
		case KEY_PAGE_DOWN:
			if (c == KEY_PAGE_UP && E.cy != 0) {
				E.cy = 0;
			}
			else if (c == KEY_PAGE_DOWN && E.cy != E.screenrows-1) {
				E.cy = E.screenrows-1;
			}
			{
				int times = E.screenrows;
				while (times--) {
					ret_c_editor_move_cursor(c == KEY_PAGE_UP ? KEY_UP : KEY_DOWN);
				}
			}
			break;
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
			ret_c_editor_move_cursor(c);
			break;
		case KEY_ESC:
			E.running = FALSE;
            break;
		case KEY_TAB:
				for (int i = 0; i < E.spaces_for_tab; ++i)
					ret_c_editor_insert_char(' ');
			break;
		default:
			ret_c_editor_insert_char(c);
			break;
    }
}

void ret_c_editor_wait_for_keypress() {
	RETPrintLine("Press key to continue.");
	RETRenderFrame();
	ret_c_editor_read_key();
}

// -----------------------------------------------------------------------------
#pragma mark - Editor init and loop

BOOL ret_c_editor_is_running() {
	return E.running;
}

void ret_c_editor_init() {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
	E.path = NULL;
    E.syntax = NULL;
	E.screencols = RETGetColumns();
	E.screenrows = RETGetRows();
    E.spaces_for_tab = 2;
    
	// For statusbar
    E.screenrows -= 2;
}

void ret_c_editor_load(char *filename, char *extension, char *path) {
	ret_c_editor_select_syntax_highlight(extension);
	ret_c_editor_open(filename, extension, path);
}

void ret_c_editor_start(char *filename, char *extension, char *path) {
    ret_text_cursor_show(FALSE);
    
    ret_c_editor_init();
	
	if (filename != NULL) {
		ret_c_editor_select_syntax_highlight(extension);
		ret_c_editor_open(filename, extension, path);
	}
	
	ret_c_editor_set_status_message("F2 = SAVE | F3 = BUILD&RUN");
    
	E.running = TRUE;
	
	while (E.running) {
        ret_c_editor_refresh_screen();
        ret_c_editor_process_keypress();
    }
    
    ret_text_clear_screen();
    ret_text_cursor_set(0, 0);
    ret_text_cursor_show(TRUE);
}

void ret_c_editor_change_size(int cols, int rows) {
    ret_text_initialize(cols, rows);

    E.screencols = RETGetColumns();
    E.screenrows = RETGetRows();
    E.screenrows -= 2;

    E.cx = 0;
    E.cy = 0;

    ret_c_editor_refresh_screen();
}
