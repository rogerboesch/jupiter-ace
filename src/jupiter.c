/* Miscellaneous glue code for iACE, copyright (C) 2012 Edward Patel.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <unistd.h>
#include <ctype.h>

//#include "SoundGenerator.h"

#include "rb_types.h"
#include "rb_platform.h"
#include "rb_keycodes.h"
#include "jupiter_keys.h"

#include "ace.rom.h"
#include "frogger.h"
#include "z80.h"

// --- External functions ------------------------------------------------------

extern void z80_init(void);
extern void z80_frame(void);
extern void get_z80_internal_state(char **ptr, int *len);
extern void set_z80_internal_state(const char *ptr);

extern void platform_set_pixel(int x, int y, UINT32 color);
extern void platform_render_frame(void);
extern int  platform_get_key(void);
extern void platform_exit(void);
extern void platform_sleep(UINT32 ms);

// --- Forward functions -------------------------------------------------------

BOOL refresh(void);
void keyboard_clear();

// --- Globals -----------------------------------------------------------------

static BOOL quit = FALSE;
static BYTE video_ram_old[32*24];

unsigned long tstates=0, tsmax=65000, tsmaxfreq=50;
volatile int interrupted = 0;
int reset_ace = 0;
static BYTE keyboard_ports[8];

int memattr[8] = {0,1,1,1,1,1,1,1}; /* 8K RAM Banks */

BYTE mem[65536];
BYTE *memptr[8] = {
    mem,
    mem+0x2000,
    mem+0x4000,
    mem+0x6000,
    mem+0x8000,
    mem+0xa000,
    mem+0xc000,
    mem+0xe000
};

BYTE empty_bytes[799] = {       /* a small screen       */
    0x1a,0x00,0x20,0x6f,        /* will be loaded if    */
    0x74,0x68,0x65,0x72,        /* wanted file can't be */
    0x20,0x20,0x20,0x20,        /* opened               */
    0x20,0x00,0x03,0x00,
    0x24,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,
    0x01,0x03,0x43,0x6f,
    0x75,0x6c,0x64,0x6e,
    0x27,0x74,0x20,0x6c,
    0x6f,0x61,0x64,0x20,
    0x79,0x6f,0x75,0x72,
    0x20,0x66,0x69,0x6c,
    0x65,0x21,0x20,
};

BYTE empty_dict[] = {           /* a small forth program */
    0x1a,0x00,0x00,0x6f,        /* will be loaded if     */
    0x74,0x68,0x65,0x72,        /* wanted file can't be  */
    0x20,0x20,0x20,0x20,        /* opened                */
    0x20,0x2a,0x00,0x51,
    0x3c,0x58,0x3c,0x4c,
    0x3c,0x4c,0x3c,0x4f,
    0x3c,0x7b,0x3c,0x20,
    0x2b,0x00,0x52,0x55,
    0xce,0x27,0x00,0x49,
    0x3c,0x03,0xc3,0x0e,
    0x1d,0x0a,0x96,0x13,
    0x18,0x00,0x43,0x6f,
    0x75,0x6c,0x64,0x6e,
    0x27,0x74,0x20,0x6c,
    0x6f,0x61,0x64,0x20,
    0x79,0x6f,0x75,0x72,
    0x20,0x66,0x69,0x6c,
    0x65,0x21,0xb6,0x04,
    0xff,0x00
};

// --- State persistency -------------------------------------------------------

void save_p(int _de, int _hl) {
    char filename[64];
    int i;
    static int firstTime=1;    
    
    if (firstTime) {
        //_data = [NSMutableData data];
        
        i=0;
        while (!isspace(mem[_hl+1+i]) && i<10) {
            filename[i]=mem[_hl+1+i];
            i++;
        }
        filename[i++]='.';
        if (mem[8961]) { /* dict or bytes save */
            filename[i++]='b';
            filename[i++]='y';
            filename[i++]='t';
        } else {
            filename[i++]='d';
            filename[i++]='i';
            filename[i++]='c';
        }
        filename[i++]='\0';
        //_filename = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
        _de++;
        char bytes[2] = {
            _de&0xff,
            (_de>>8)&0xff,
        };

        platform_dbg("Save file is not yet implemented");

        //[_data appendBytes:bytes length:sizeof(bytes)];
        //[_data appendBytes:&mem[_hl] length:_de];
        
        firstTime = 0;
    }
    else {
        _de++;
        char bytes[2] = {
            _de&0xff,
            (_de>>8)&0xff,
        };

        //[_data appendBytes:bytes length:sizeof(bytes)];
        //[_data appendBytes:&mem[_hl] length:_de];
        firstTime = 1;
        
        //tapes[_filename] = _data;

        //NSString *dir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
        //[tapes writeToFile:[dir stringByAppendingString:@"/tapes.dic"] atomically:YES];

        //_filename = nil;
        //_data = nil;
    }
}

void load_p(int _de, int _hl) {
    char filename[64];
    int i;
    static BYTE *empty_tape;
    static int efp;
    static int firstTime=1;
    static const BYTE *ptr;
    static BYTE* data;

    if (firstTime) {
        i=0;

        while (!isspace(mem[9985+1+i])&&i<10) {
            filename[i]=mem[9985+1+i]; i++;
        }

        filename[i++]='.';
        if (mem[9985]) { /* dict or bytes load */
            filename[i++]='b';
            filename[i++]='y';
            filename[i++]='t';
            empty_tape = empty_bytes;
        } else {
            filename[i++]='d';
            filename[i++]='i';
            filename[i++]='c';
            empty_tape = empty_dict;
        }

        filename[i++]='\0';

        // TODO: Check for filename and apply to data
        //_filename = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
        //_data = tapes[_filename];
        data = FROGGER_DIC;
        platform_dbg("Whatever filename you choose, right now, 'Frogger' is loaded");

        if (!data) {
            efp=0;
            _de=empty_tape[efp++];
            _de+=256*empty_tape[efp++];
            memcpy(&mem[_hl],&empty_tape[efp],_de-1); /* -1 -> skip last byte */
            
            for (i=0;i<_de;i++) /* get memory OK */
                store(_hl+i,fetch(_hl+i));
            
            efp+=_de;
        
            for (i=0;i<10;i++)               /* let this file be it! */
            mem[_hl+1+i]=mem[9985+1+i];
        
            firstTime = 0;
        }
        else {
            ptr = data;
            _de = *ptr++;
            _de += 256*(*ptr++);
            memcpy(&mem[_hl],ptr,_de-1); /* -1 -> skip last byte */
            ptr += _de;
        
            for (i=0;i<_de;i++) /* get memory OK */
                store(_hl+i,fetch(_hl+i));
        
            for (i=0;i<10;i++)               /* let this file be it! */
               mem[_hl+1+i]=mem[9985+1+i];
    
               firstTime = 0;
        }
    }
    else {
        if (data) {
            _de = *ptr++;
            _de += 256*(*ptr++);
            memcpy(&mem[_hl],ptr,_de-1); /* -1 -> skip last byte */
            for (i=0;i<_de;i++) /* get memory OK */
                store(_hl+i,fetch(_hl+i));
            data = NULL;
        }
        else {
            _de=empty_tape[efp++];
            _de+=256*empty_tape[efp++];
            memcpy(&mem[_hl],&empty_tape[efp],_de-1); /* -1 -> skip last byte */
            for (i=0;i<_de;i++) /* get memory OK */
                store(_hl+i,fetch(_hl+i));
        }

        firstTime = 1;
    }
}

void load_state() {
    // TODO: Implement
}

void save_state(){
    // TODO: Implement
}

// --- Spooler -----------------------------------------------------------------

static char *spooling_string = NULL;
static char *spooling_string_pos = NULL;

int spooler_is_active() {
    return spooling_string ? 1 : 0;
}

int spooler_read_char() {
    int rc = 0;

    if (spooling_string_pos) {
        rc = *spooling_string_pos++;

        if (!*spooling_string_pos) {
            free(spooling_string);

            spooling_string = NULL;
            spooling_string_pos = NULL;
        }
    }

    return rc;
}

void spooler_add_str(char* str) {
    if (spooling_string) {
        free(spooling_string);
    }

    spooling_string = strdup(str);
    spooling_string_pos = spooling_string;
    
    keyboard_clear();
}

// --- Keyboard & Sound --------------------------------------------------------

typedef enum {
    AceKey_none,
    AceKey_DeleteLine, AceKey_InverseVideo, AceKey_Graphics,
    AceKey_Left, AceKey_Down, AceKey_Up, AceKey_Right,
    AceKey_Delete = 0x08,
    AceKey_1 = 0x31, AceKey_2 = 0x32, AceKey_3 = 0x33, AceKey_4 = 0x34,
    AceKey_5 = 0x35, AceKey_6 = 0x36, AceKey_7 = 0x37, AceKey_8 = 0x38,
    AceKey_9 = 0x39, AceKey_0 = 0x30,
    AceKey_exclam = 0x21, AceKey_at = 0x40, AceKey_numbersign = 0x23,
    AceKey_dollar = 0x24, AceKey_percent = 0x25,
    AceKey_Break,
    AceKey_ampersand = 0x26, AceKey_apostrophe = 0x27,
    AceKey_parenleft = 0x28, AceKey_parenright = 0x29,
    AceKey_underscore = 0x5F,
    AceKey_A = 0x41, AceKey_a = 0x61, AceKey_B = 0x42, AceKey_b = 0x62,
    AceKey_C = 0x43, AceKey_c = 0x63, AceKey_D = 0x44, AceKey_d = 0x64,
    AceKey_E = 0x45, AceKey_e = 0x65, AceKey_F = 0x46, AceKey_f = 0x66,
    AceKey_G = 0x47, AceKey_g = 0x67, AceKey_H = 0x48, AceKey_h = 0x68,
    AceKey_I = 0x49, AceKey_i = 0x69, AceKey_J = 0x4A, AceKey_j = 0x6A,
    AceKey_K = 0x4B, AceKey_k = 0x6B, AceKey_L = 0x4C, AceKey_l = 0x6C,
    AceKey_M = 0x4D, AceKey_m = 0x6D, AceKey_N = 0x4E, AceKey_n = 0x6E,
    AceKey_O = 0x4F, AceKey_o = 0x6F, AceKey_P = 0x50, AceKey_p = 0x70,
    AceKey_Q = 0x51, AceKey_q = 0x71, AceKey_R = 0x52, AceKey_r = 0x72,
    AceKey_S = 0x53, AceKey_s = 0x73, AceKey_T = 0x54, AceKey_t = 0x74,
    AceKey_U = 0x55, AceKey_u = 0x75, AceKey_V = 0x56, AceKey_v = 0x76,
    AceKey_W = 0x57, AceKey_w = 0x77, AceKey_X = 0x58, AceKey_x = 0x78,
    AceKey_Y = 0x59, AceKey_y = 0x79, AceKey_Z = 0x5A, AceKey_z = 0x80,
    AceKey_less = 0x3C, AceKey_greater = 0x3E,
    AceKey_bracketleft = 0x5B, AceKey_bracketright = 0x5D,
    AceKey_copyright = 0x60,
    AceKey_semicolon = 0x3B, AceKey_quotedbl = 0x22, AceKey_asciitilde = 0x7E,
    AceKey_bar = 0x7C, AceKey_backslash = 0x5C,
    AceKey_braceleft = 0x7B, AceKey_braceright = 0x7D,
    AceKey_asciicircum = 0x5E,
    AceKey_minus = 0x2D, AceKey_plus = 0x2B, AceKey_equal = 0x3D,
    AceKey_Return = 0x0A,
    AceKey_colon = 0x3A, AceKey_sterling = 0x9C, AceKey_question = 0x3F,
    AceKey_slash = 0x2F, AceKey_asterisk = 0x2A, AceKey_comma = 0x2C,
    AceKey_period = 0x2E,
    AceKey_space = 0x20,
    AceKey_Tab = 0x09
} AceKey;

/* key, keyport_index, and_value, keyport_index, and_value
 * if keyport_index == -1 then no action for that port */
static const int keypress_response[] = {
    AceKey_DeleteLine, 3, 0xfe, 0, 0xfe,
    AceKey_InverseVideo, 3, 0xf7, 0, 0xfe,
    AceKey_Graphics, 4, 0xfd, 0, 0xfe,
    AceKey_Left, 3, 0xef, 0, 0xfe,
    AceKey_Down, 4, 0xf7, 0, 0xfe,
    AceKey_Up, 4, 0xef, 0, 0xfe,
    AceKey_Right, 4, 0xfb, 0, 0xfe,
    AceKey_Delete, 0, 0xfe, 4, 0xfe,
    AceKey_1, 3, 0xfe, -1, 0,
    AceKey_2, 3, 0xfd, -1, 0,
    AceKey_3, 3, 0xfb, -1, 0,
    AceKey_4, 3, 0xf7, -1, 0,
    AceKey_5, 3, 0xef, -1, 0,
    AceKey_6, 4, 0xef, -1, 0,
    AceKey_7, 4, 0xf7, -1, 0,
    AceKey_8, 4, 0xfb, -1, 0,
    AceKey_9, 4, 0xfd, -1, 0,
    AceKey_0, 4, 0xfe, -1, 0,
    AceKey_exclam, 3, 0xfe, 0, 0xfd,
    AceKey_at, 3, 0xfd, 0, 0xfd,
    AceKey_numbersign, 3, 0xfb, 0, 0xfd,
    AceKey_dollar, 3, 0xf7, 0, 0xfd,
    AceKey_percent, 3, 0xef, 0, 0xfd,
    AceKey_Break, 7, 0xfe, 0, 0xfe,     /* Break */
    AceKey_ampersand, 4, 0xef, 0, 0xfd,
    AceKey_apostrophe, 4, 0xf7, 0, 0xfd,
    AceKey_parenleft, 4, 0xfb, 0, 0xfd,
    AceKey_parenright, 4, 0xfd, 0, 0xfd,
    AceKey_underscore, 4, 0xfe, 0, 0xfd,
    AceKey_A, 0, 0xfe, 1, 0xfe,
    AceKey_a, 1, 0xfe, -1, 0,
    AceKey_B, 0, 0xfe, 7, 0xf7,
    AceKey_b, 7, 0xf7, -1, 0,
    AceKey_C, 0, 0xee, -1, 0,
    AceKey_c, 0, 0xef, -1, 0,
    AceKey_D, 0, 0xfe, 1, 0xfb,
    AceKey_d, 1, 0xfb, -1, 0,
    AceKey_E, 0, 0xfe, 2, 0xfb,
    AceKey_e, 2, 0xfb, -1, 0,
    AceKey_F, 0, 0xfe, 1, 0xf7,
    AceKey_f, 1, 0xf7, -1, 0,
    AceKey_G, 0, 0xfe, 1, 0xef,
    AceKey_g, 1, 0xef, -1, 0,
    AceKey_H, 0, 0xfe, 6, 0xef,
    AceKey_h, 6, 0xef, -1, 0,
    AceKey_I, 0, 0xfe, 5, 0xfb,
    AceKey_i, 5, 0xfb, -1, 0,
    AceKey_J, 0, 0xfe, 6, 0xf7,
    AceKey_j, 6, 0xf7, -1, 0,
    AceKey_K, 0, 0xfe, 6, 0xfb,
    AceKey_k, 6, 0xfb, -1, 0,
    AceKey_L, 0, 0xfe, 6, 0xfd,
    AceKey_l, 6, 0xfd, -1, 0,
    AceKey_M, 0, 0xfe, 7, 0xfd,
    AceKey_m, 7, 0xfd, -1, 0,
    AceKey_N, 0, 0xfe, 7, 0xfb,
    AceKey_n, 7, 0xfb, -1, 0,
    AceKey_O, 0, 0xfe, 5, 0xfd,
    AceKey_o, 5, 0xfd, -1, 0,
    AceKey_P, 0, 0xfe, 5, 0xfe,
    AceKey_p, 5, 0xfe, -1, 0,
    AceKey_Q, 0, 0xfe, 2, 0xfe,
    AceKey_q, 2, 0xfe, -1, 0,
    AceKey_R, 0, 0xfe, 2, 0xf7,
    AceKey_r, 2, 0xf7, -1, 0,
    AceKey_S, 0, 0xfe, 1, 0xfd,
    AceKey_s, 1, 0xfd, -1, 0,
    AceKey_T, 0, 0xfe, 2, 0xef,
    AceKey_t, 2, 0xef, -1, 0,
    AceKey_U, 0, 0xfe, 5, 0xf7,
    AceKey_u, 5, 0xf7, -1, 0,
    AceKey_V, 0, 0xfe, 7, 0xef,
    AceKey_v, 7, 0xef, -1, 0,
    AceKey_W, 0, 0xfe, 2, 0xfd,
    AceKey_w, 2, 0xfd, -1, 0,
    AceKey_X, 0, 0xf6, -1, 0,
    AceKey_x, 0, 0xf7, -1, 0,
    AceKey_Y, 0, 0xfe, 5, 0xef,
    AceKey_y, 5, 0xef, -1, 0,
    AceKey_Z, 0, 0xfa, -1, 0,
    AceKey_z, 0, 0xfb, -1, 0,
    AceKey_less, 2, 0xf7, 0, 0xfd,
    AceKey_greater, 2, 0xef, 0, 0xfd,
    AceKey_bracketleft, 5, 0xef, 0, 0xfd,
    AceKey_bracketright, 5, 0xf7, 0, 0xfd,
    AceKey_copyright, 5, 0xfb, 0, 0xfd,
    AceKey_semicolon, 5, 0xfd, 0, 0xfd,
    AceKey_quotedbl, 5, 0xfe, 0, 0xfd,
    AceKey_asciitilde, 1, 0xfe, 0, 0xfd,
    AceKey_bar, 1, 0xfd, 0, 0xfd,
    AceKey_backslash, 1, 0xfb, 0, 0xfd,
    AceKey_braceleft, 1, 0xf7, 0, 0xfd,
    AceKey_braceright, 1, 0xef, 0, 0xfd,
    AceKey_asciicircum, 6, 0xef, 0, 0xfd,
    AceKey_minus, 6, 0xf7, 0, 0xfd,
    AceKey_plus, 6, 0xfb, 0, 0xfd,
    AceKey_equal, 6, 0xfd, 0, 0xfd,
    AceKey_Return, 6, 0xfe, -1, 0,
    AceKey_colon, 0, 0xf9, -1, 0,
    AceKey_sterling, 0, 0xf5, -1, 0,
    AceKey_question, 0, 0xed, -1, 0,
    AceKey_slash, 7, 0xef, 0, 0xfd,
    AceKey_asterisk, 7, 0xf7, 0, 0xfd,
    AceKey_comma, 7, 0xfb, 0, 0xfd,
    AceKey_period, 7, 0xfd, 0, 0xfd,
    AceKey_space, 7, 0xfe, -1, 0,
    AceKey_Tab, 7, 0xfe, -1, 0,
};

static int keyboard_get_key_response(AceKey aceKey, int *keyport1, int *keyport2, int *keyport1_response, int *keyport2_response) {
    int i;
    int num_keys = sizeof(keypress_response)/sizeof(keypress_response[0]);
    
    for (i = 0; i < num_keys; i+= 5) {
        if (keypress_response[i] == aceKey) {
            *keyport1 = keypress_response[i+1];
            *keyport2 = keypress_response[i+3];
            *keyport1_response = keypress_response[i+2];
            *keyport2_response = keypress_response[i+4];
            return 1;
        }
    }
    return 0;
}

static void keyboard_process_keypress_keyports(AceKey aceKey) {
    int key_found;
    int keyport1, keyport2;
    int keyport1_and_value, keyport2_and_value;
    
    key_found = keyboard_get_key_response(aceKey, &keyport1, &keyport2,
                                          &keyport1_and_value, &keyport2_and_value);
    if (key_found) {
        keyboard_ports[keyport1] &= keyport1_and_value;
        if (keyport2 != -1)
            keyboard_ports[keyport2] &= keyport2_and_value;
    }
}

static void keyboard_process_keyrelease_keyports(AceKey aceKey) {
    int key_found;
    int keyport1, keyport2;
    int keyport1_or_value, keyport2_or_value;
    
    key_found = keyboard_get_key_response(aceKey, &keyport1, &keyport2, &keyport1_or_value, &keyport2_or_value);
    if (key_found) {
        keyboard_ports[keyport1] |= ~keyport1_or_value;
        if (keyport2 != -1)
            keyboard_ports[keyport2] |= ~keyport2_or_value;
    }
}

void keyboard_keypress(int aceKey) {
    keyboard_process_keypress_keyports(aceKey);
}

void keyboard_keyrelease(int aceKey) {
    keyboard_process_keyrelease_keyports(aceKey);
}

BYTE keyboard_get_keyport(int port) { return keyboard_ports[port]; }

void keyboard_clear() {
    for (int i=0; i < 8; i++) {
        keyboard_ports[i] = 0xff;
    }
}

// --- IN/OUT ------------------------------------------------------------------

static BOOL soundStarted = FALSE;
static long sound_tsstate = 0;

unsigned int in(int h, int l) {
    static int scans_left_before_next = 0;
    
    if (l==0xfe && h==0xfe && !scans_left_before_next) {
        static int x = 0;
        
        if (x) {
            keyboard_clear();
            if (x==0x0a)
                scans_left_before_next = 4;
            x = 0;
        }
        else {
            x = spooler_read_char();
            if (x) {
                keyboard_keypress(x);
                scans_left_before_next = 4;
            }
        }
        
    }
    else if (l==0xfe && h==0xfd && scans_left_before_next) {
        scans_left_before_next--;
    }

    if (l==0xfe) {        
        if (!soundStarted) {
            // TODO: Implement
        }
        soundStarted = FALSE;
        
        switch (h) {
            case 0xfe:
                return keyboard_get_keyport(0);
            case 0xfd:
                return keyboard_get_keyport(1);
            case 0xfb:
                return keyboard_get_keyport(2);
            case 0xf7:
                return keyboard_get_keyport(3);
            case 0xef:
                return keyboard_get_keyport(4);
            case 0xdf:
                return keyboard_get_keyport(5);
            case 0xbf:
                return keyboard_get_keyport(6);
            case 0x7f:
                return keyboard_get_keyport(7);
            default:
                return 255;
        }
    }
    return 255;
}

unsigned int out(int h, int l, int a) {
    if (l==0xfe) {
        // TODO: Implement
        soundStarted = TRUE;
        long dt = (long)tstates-sound_tsstate;
        if (dt < 0)
            dt += tsmax;
        int n = (float)dt/(tsmax*tsmaxfreq/22050);
        sound_tsstate = tstates;
    }

    return 0;
}

int process_key(int code) {
    switch (code) {
        case KEY_SHIFT: return JUPITER_KEY_SHIFT;
        case KEY_ALT: return JUPITER_KEY_SYMSHIFT;
        case KEY_SPACE: return JUPITER_KEY_SPACE;
        case KEY_ENTER: return JUPITER_KEY_ENTER;

        // Individual Digit Keys
        case KEY_0: return JUPITER_KEY_0;
        case KEY_1: return JUPITER_KEY_1;
        case KEY_2: return JUPITER_KEY_2;
        case KEY_3: return JUPITER_KEY_3;
        case KEY_4: return JUPITER_KEY_4;
        case KEY_5: return JUPITER_KEY_5;
        case KEY_6: return JUPITER_KEY_6;
        case KEY_7: return JUPITER_KEY_7;
        case KEY_8: return JUPITER_KEY_8;
        case KEY_9: return JUPITER_KEY_9;

        // Alphabet
        case KEY_A: return JUPITER_KEY_A;
        case KEY_B: return JUPITER_KEY_B;
        case KEY_C: return JUPITER_KEY_C;
        case KEY_D: return JUPITER_KEY_D;
        case KEY_E: return JUPITER_KEY_E;
        case KEY_F: return JUPITER_KEY_F;
        case KEY_G: return JUPITER_KEY_G;
        case KEY_H: return JUPITER_KEY_H;
        case KEY_I: return JUPITER_KEY_I;
        case KEY_J: return JUPITER_KEY_J;
        case KEY_K: return JUPITER_KEY_K;
        case KEY_L: return JUPITER_KEY_L;
        case KEY_M: return JUPITER_KEY_M;
        case KEY_N: return JUPITER_KEY_N;
        case KEY_O: return JUPITER_KEY_O;
        case KEY_P: return JUPITER_KEY_P;
        case KEY_Q: return JUPITER_KEY_Q;
        case KEY_R: return JUPITER_KEY_R;
        case KEY_S: return JUPITER_KEY_S;
        case KEY_T: return JUPITER_KEY_T;
        case KEY_U: return JUPITER_KEY_U;
        case KEY_V: return JUPITER_KEY_V;
        case KEY_W: return JUPITER_KEY_W;
        case KEY_X: return JUPITER_KEY_X;
        case KEY_Y: return JUPITER_KEY_Y;
        case KEY_Z: return JUPITER_KEY_Z;

        default: return 0;
    }
}

// --- Interrupt & TStatae handling  -------------------------------------------

void fix_tstates() {
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
        load_state();
        keyboard_clear();
    }

    static double t0 = 0.0;
    double t1 = platform_get_ms();
    tstates -= tsmax;
    interrupted = 1;
    double delay = 0.020 - (t1 - t0);

    t0 = t1;

    if (delay > 0.0) {
        platform_sleep((UINT32)(delay*1000));
    }
}

void do_interrupt() {
    static int scrn_freq = 2;
    static int count = 0;
 
    if (quit) {
        return;
    }
        
    if (interrupted == 1) {
        interrupted = 2;

        int key = platform_get_key();

        if (key == KEY_ESC) {
            quit = TRUE;
            interrupted = 0;

            platform_log("Escape key pressed, quit");

            return;
        }
        else if (key == KEY_TAB) {
            platform_log("TAB key pressed, RESET");
            reset_ace = 1;
        }
        else if (key == KEY_1) {
            spooler_add_str("load frogger\n");
        }
        else if (key == KEY_2) {
            spooler_add_str("go\n");
        }
        else if (key > 0) {
            int code = process_key(key);
            platform_dbg("Key %d => Jupiter code %d", key, code);

            if (code > 0) {
                keyboard_ports[0xff & code] &= ~(code>>8);
            }

        }

        /* only do refresh() every 1/Nth */
        count++;
        if (count >= scrn_freq) {
            count=0;
            //spooler_read();
            if (refresh())
                platform_render_frame();
        }
    }
}

// --- Screen update -----------------------------------------------------------

void set_image_character(int x, int y, int inv, BYTE *charbmap) {
    UINT32 color;
    int charbmap_x, charbmap_y;
    BYTE charbmap_row;
    BYTE charbmap_row_mask;
    
    for (charbmap_y = 0; charbmap_y < 8; charbmap_y++) {
        charbmap_row = charbmap[charbmap_y];
        
        if (inv) {
            charbmap_row ^= 255;
        }

        charbmap_row_mask = 0x80;
        
        for (charbmap_x = 0; charbmap_x < 8; charbmap_x++) {
            color = !(charbmap_row & charbmap_row_mask) ? 0x00000000 : 0xffffffff;
            platform_set_pixel(x*8+charbmap_x, y*8+charbmap_y, color);
            charbmap_row_mask >>= 1;
        }
    }
}

BOOL refresh() {
    BYTE *video_ram, *charset;
    int x, y, c;
    
    charset = mem+0x2c00;
    video_ram = mem+0x2400;
    
    if (!memcmp(video_ram, video_ram_old, sizeof(video_ram_old))) {
        return FALSE;
    }

    for (y=0; y<24; y++) {
        for (x=0; x<32; x++) {
            c = video_ram[x+y*32];
            video_ram_old[x+y*32] = c;
            set_image_character(x, y, c&0x80, charset + (c&0x7f)*8);
        }
    }

    return TRUE;
}

// --- Main emulatoir loop -----------------------------------------------------

void patch_rom(BYTE *mem) {
    mem[0x18a7]=0xed; /* for load_p */
    mem[0x18a8]=0xfc;
    mem[0x18a9]=0xc9;
    
    mem[0x1820]=0xed; /* for save_p */
    mem[0x1821]=0xfd;
    mem[0x1822]=0xc9;
}

void setup() {
    memcpy(mem, ace_rom, ace_rom_len);
    patch_rom(mem);
    memset(mem+8192, 0xff, sizeof(mem)-8192);
    keyboard_clear();

    memset(video_ram_old, 0xff, 768);

    // TODO: Implement tape list
}

int jupiter_main_loop(void) {
    // Initialize jupiter
    setup();
    z80_init();
    refresh();
    
    while (!quit) {
        // Emulator step
        z80_frame();
    }

    return 0;
}

