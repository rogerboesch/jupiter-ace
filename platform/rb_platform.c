/*
 * =============================================================================
 *  VEC3X: Laser Arcade
 *  Copyright © 2025 Roger Boesch — All rights reserved.
 *
 *    V     V   EEEEEEE   CCCCC     33333    X     X
 *    V     V   E        C     C       33     X   X 
 *    V     V   EEEEEEE  C             333     X X  
 *     V   V    E        C               33     X X  
 *      V V     E        C     C         33    X   X 
 *       V      EEEEEEE   CCCCC      33333    X     X
 *                           VEC3X — RETRO ARCADE ENGINE
 *
 *  Module : Platform code
 * -----------------------------------------------------------------------------
 *  Description:
 *    Provides some platform code (mainly log at the moment)
 *
 *  Key Functions:
 *    • platform_log(const char *fmt, ...)  — prints to stdout
 *    • platform_dbg(const char *fmt, ...)  — prints to stdout
 *    • platform_err(const char *fmt, ...)  — prints to stderr
 * =============================================================================
 */

 #include "rb_platform.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

// ----------------------------- Log helpers -----------------------------------

static ConsoleFn s_console = NULL;

static void platform_write(int severity, char* msg) {
    if (s_console == NULL) {
        switch (severity) {
            case LOG_SEV_DEBUG:
                fprintf(stdout, "> %s\n", msg);
                break;
            case LOG_SEV_DUMP:
                fprintf(stdout, "%s\n", msg);
                break;
            case LOG_SEV_INFO:
                fprintf(stdout, "INF: %s\n", msg);
                break;
            case LOG_SEV_ERROR:
                fprintf(stderr, "ERR: %s\n", msg);
                break;
            default:
                break;
            }
    }
    else {
        s_console(severity, msg);
    }
}

void platform_dbg(const char *fmt, ...) {
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    buf[1023] = 0;

    platform_write(LOG_SEV_DEBUG, buf);
}

void platform_log(const char *fmt, ...) {
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    buf[1023] = 0;

    platform_write(LOG_SEV_INFO, buf);
}

void platform_err(const char *fmt, ...) {
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    buf[1023] = 0;

    platform_write(LOG_SEV_ERROR, buf);
}

void platform_dump_stop(void) {
    platform_write(LOG_SEV_DUMP_STOP, "");
}

// --------------------------- Console helpers ---------------------------------

void platform_set_console(ConsoleFn console) {
    s_console = console;
}

void platform_dump(const char *fmt, ...) {
    if (s_console == NULL) return;

    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    buf[1023] = 0;

    s_console(LOG_SEV_DUMP, buf);
}

// -------------------------- Time helpers -------------------------------------

static double _start = -99.0;

void platform_delay(unsigned int ms) {
#ifdef __APPLE__
    // TODO: 
    usleep(ms * 1000); 
#endif
}

double platform_get_ms(void) {
#ifdef __APPLE__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double time = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;

    if (_start < 0.0) _start = time;

    return time-_start;
#else
    // TODO: implement for other platforms
    return 0.0;
#endif
}

double _last = -99.0;

int platform_get_delta_ms(void) {
    if (_last < 0.0) {
        _last = platform_get_ms();
        return 0;
    }

    double delta = platform_get_ms() - _last;
    _last = platform_get_ms();

    int ms = (int)(delta*1000.0);
    return ms;
}

void platform_delay_ms(unsigned int ms) {
    // TODO: usleep(ms * 1000); 
}

// ------------------------------ Path helpers ---------------------------------

int platform_resource_file_path(char* path, int size, const char* filename, const char* extension) {
    char* homeDir = getenv("HOME");
    snprintf(path, size, "%s/%s.%s", homeDir, filename, extension);

    return 1;
}

int platform_get_asset_folder(char* path, int size) {
    char* homeDir = getenv("HOME");
    snprintf(path, size, "%s/%s/", homeDir, "vectrex-roms");

    return 1;

}
