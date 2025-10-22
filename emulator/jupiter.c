
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "rb_types.h"
#include "rb_platform.h"
#include "rb_keycodes.h"
#include "jupiter_keys.h"

#include "rom_ace.h"
#include "z80.h"

// --- External functions ------------------------------------------------------

extern void z80_init(void);
extern void z80_frame(void);
extern void get_z80_internal_state(char **ptr, int *len);
extern void set_z80_internal_state(const char *ptr);

extern void spooler_add_str(char* str);
extern int spooler_read_char();

extern void keyboard_process(void);
extern void keyboard_clear(void);
extern BYTE keyboard_get_keyport(int port);
extern void keyboard_keypress(int aceKey);

extern void platform_set_pixel(int x, int y, UINT32 color);
extern void platform_render_frame(void);
extern void platform_exit(void);
extern void platform_sleep(UINT32 ms);

// --- Forward functions -------------------------------------------------------

BOOL refresh(BOOL force_refresh);
void print_welcome(void);

// --- Globals -----------------------------------------------------------------

BOOL quit = FALSE;
static BYTE video_ram_old[32*24];

unsigned long tstates=0, tsmax=65000, tsmaxfreq=50;
volatile int interrupted = 0;
int reset_ace = 0;

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

// --- State persistency -------------------------------------------------------

void load_state() {
    // TODO: Implement
}

void save_state(){
    // TODO: Implement
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

// --- Interrupt & TState handling  --------------------------------------------

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
    static BOOL first_time = TRUE;

    if (quit) {
        return;
    }
        
    if (interrupted == 1) {
        interrupted = 2;

        keyboard_process();

        /* only do refresh() every 1/Nth */
        count++;
        if (count >= scrn_freq) {
            count=0;

            //spooler_read();
            if (refresh(FALSE)) {
                platform_render_frame();

                if (first_time) {
                    print_welcome();
                    first_time = FALSE;
                }
            }
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

BOOL refresh(BOOL force_refresh) {
    BYTE *video_ram, *charset;
    int x, y, c;
    
    charset = mem+0x2c00;
    video_ram = mem+0x2400;
    
    if (!memcmp(video_ram, video_ram_old, sizeof(video_ram_old)) && !force_refresh) {
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

// --- Write any text in the pixel buffer --------------------------------------

void print_char(char ch, int x, int y) {
    BYTE* charset = mem+0x2c00;

    set_image_character(x, y, ch&0x80, charset + (ch&0x7f)*8);
}

void print_str(char* str, int x, int y) {
    char* c = str;
    while (*c) {
        print_char(*c++, x, y);
        x++;
    }

    platform_render_frame();
}

void print_char_set() {
    int x=0, y=1;
    for (int i=0; i<256; i++) {
        print_char(i, x, y);
        x++;

        if (x>=32) {
            x=0;
            y++;
        }
    }

    platform_render_frame();
}

void print_welcome(void) {
    print_str("JUPITER ACE EMU 0.4", 1, 1);
    print_str("\x7f 2025 BY ROGER BOESCH", 1, 2);

    platform_dbg("Prnt welcome");
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
    memcpy(mem, rom_ace, rom_ace_len);
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

    while (!quit) {
        // Emulator step
        z80_frame();
    }

    return 0;
}

