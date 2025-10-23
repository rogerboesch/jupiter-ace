
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>

#include "z80.h"
#include "rom_frogger.h"

#include "rb_types.h"
#include "rb_platform.h"

// --- Functions ---------------------------------------------------------------

BOOL save_bytes(char* filename, BYTE* data, int size);

// --- Globals -----------------------------------------------------------------

extern BYTE mem[65536];

// A small forth program will be loaded if file cant be opened
BYTE empty_dict[] = {           
    0x1a,0x00,0x00,0x6f,        
    0x74,0x68,0x65,0x72,        
    0x20,0x20,0x20,0x20,        
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

// A small screen will be loaded if file cant be opened
BYTE empty_bytes[799] = {       
    0x1a,0x00,0x20,0x6f,        
    0x74,0x68,0x65,0x72,        
    0x20,0x20,0x20,0x20,        
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

// --- Save/Load ---------------------------------------------------------------

void save_p(int _de, int _hl) {
    char filename[64];
    static int firstTime=1;    
    static BYTE data[3*1024];
    static int data_offset = 0;
    
    if (firstTime) {
        data_offset = 0;
        //_data = [NSMutableData data];
        int i=0;

        while (!isspace(mem[_hl+1+i]) && i<10) {
            filename[i]=mem[_hl+1+i];
            i++;
        }

        filename[i++]='.';

        // ace or byt extensions
        if (mem[8961]) { 
            filename[i++]='b';
            filename[i++]='y';
            filename[i++]='t';
        }
        else {
            filename[i++]='a';
            filename[i++]='c';
            filename[i++]='e';
        }
        filename[i++]='\0';

        _de++;
        char bytes[2] = {
            _de&0xff,
            (_de>>8)&0xff,
        };

        memcpy(data+data_offset, bytes, sizeof(bytes));
        data_offset += sizeof(bytes);
        
        memcpy(data+data_offset, &mem[_hl], _de);
        data_offset += _de;

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

        memcpy(data+data_offset, bytes, sizeof(bytes));
        data_offset += sizeof(bytes);
        
        memcpy(data+data_offset, &mem[_hl], _de);
        data_offset += _de;

        //[_data appendBytes:bytes length:sizeof(bytes)];
        //[_data appendBytes:&mem[_hl] length:_de];
        firstTime = 1;
        
        if (save_bytes(filename, data, data_offset)) {
            platform_log("File written successfully to'%s'", filename);
        }

        //tapes[_filename] = _data;
        //NSString *dir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
        //[tapes writeToFile:[dir stringByAppendingString:@"/tapes.dic"] atomically:YES];
        //_filename = nil;
        //_data = nil;
    }
}

void load_p(int _de, int _hl) {
    char filename[64];
    static BYTE *empty_tape;
    static int efp;
    static BOOL firstTime = TRUE;
    static const BYTE *ptr;
    static BYTE* data;

    if (firstTime) {
        int i=0;

        while (!isspace(mem[9985+1+i])&&i<10) {
            filename[i]=mem[9985+1+i]; i++;
        }

        filename[i++]='.';

        // ace or byt extensions
        if (mem[9985]) { 
            filename[i++]='b';
            filename[i++]='y';
            filename[i++]='t';
            empty_tape = empty_bytes;
        }
        else {
            filename[i++]='a';
            filename[i++]='c';
            filename[i++]='e';
            empty_tape = empty_dict;
        }

        filename[i++]='\0';

        // TODO: Check for filename and apply to data
        //_filename = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
        //_data = tapes[_filename];
        data = rom_frogger;
        platform_dbg("Whatever filename you choose, right now, 'Frogger' is loaded");

        if (!data) {
            efp=0;
            _de=empty_tape[efp++];
            _de+=256*empty_tape[efp++];

            // -1 -> skip last byte
            memcpy(&mem[_hl],&empty_tape[efp],_de-1); 
            
            // get memory OK
            for (i=0;i<_de;i++) {
                store(_hl+i,fetch(_hl+i));
            }

            efp += _de;
        
            // let this file be it
            for (i=0;i<10;i++) {                  
                mem[_hl+1+i]=mem[9985+1+i];
            }

            firstTime = FALSE;
        }
        else {
            ptr = data;
            _de = *ptr++;
            _de += 256*(*ptr++);

            // -1 -> skip last byte
            memcpy(&mem[_hl],ptr,_de-1); 
            ptr += _de;
        
            // get memory OK
            for (i=0;i<_de;i++) {
                store(_hl+i,fetch(_hl+i));
            }

            // let this file be it!
            for (i=0;i<10;i++) {               
                mem[_hl+1+i]=mem[9985+1+i];
            }
               
            firstTime = FALSE;
        }
    }
    else {
        if (data) {
            _de = *ptr++;
            _de += 256*(*ptr++);

            // -1 -> skip last byte
            memcpy(&mem[_hl],ptr,_de-1); 

            // get memory OK
            for (int i=0;i<_de;i++) {
                store(_hl+i,fetch(_hl+i));
            }

            data = NULL;
        }
        else {
            _de=empty_tape[efp++];
            _de+=256*empty_tape[efp++];

            // -1 -> skip last byte
            memcpy(&mem[_hl],&empty_tape[efp],_de-1); 
            
            // get memory OK
            for (int i=0; i<_de; i++) {
                store(_hl+i,fetch(_hl+i));
            }
        }

        firstTime = TRUE;
    }
}

// --- Platform file I/O -------------------------------------------------------

static BOOL get_home_folder(char* path) {
    struct passwd *pw = getpwuid(getuid());

    strcpy(path, pw->pw_dir);

    return TRUE;
}

char *read_file(const char *filename, size_t *out_len) {
    char folder[100];
    get_home_folder(folder);

    platform_dbg("Home folder: %s", folder);

    char path[200];
    snprintf(path, 199, "%s/%s", folder, filename);
    platform_dbg("Path: %s", path);

    FILE *fp = fopen(path, "rb");

    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp); return NULL;
    }
    
    long sz = ftell(fp);
    if (sz < 0) {
        fclose(fp);
        return NULL;
    }
    
    rewind(fp);
    
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[n] = '\0';

    if (out_len) {
        *out_len = n;
    }
    
    return buf;
}

BOOL save_bytes(char* filename, BYTE* data, int size) {
    char folder[100];
    get_home_folder(folder);

    platform_dbg("Home folder: %s", folder);

    char path[200];
    snprintf(path, 199, "%s/%s", folder, filename);
    platform_dbg("Path: %s", path);

    FILE *fp = fopen(path, "wb");

    if (!fp) {
        return FALSE;
    }
    
    size_t bytes_written = fwrite(data, size, 1, fp);   

    if (bytes_written != 1) {
        platform_err("Error writing to file '%s'", filename);
        fclose(fp);

        return FALSE;
    }

    fclose(fp);

    return TRUE;

}
