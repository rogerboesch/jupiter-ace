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
 *    • platform_err(const char *fmt, ...)  — prints to stderr
 * =============================================================================
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log severity
#define LOG_SEV_DEBUG       1
#define LOG_SEV_DUMP        2
#define LOG_SEV_INFO        3
#define LOG_SEV_ERROR       4
#define LOG_SEV_DUMP_STOP   99      // Special code to stop dump messages

// Debug console callback: prints a value in the debug console.
typedef void (*ConsoleFn)(int severity, char* msg);

void platform_set_console(ConsoleFn console);

void platform_dbg_enable(int flag);
void platform_dbg(const char *fmt, ...);
void platform_log(const char *fmt, ...);
void platform_err(const char *fmt, ...);
void platform_dump(const char *fmt, ...);
void platform_dump_stop(void);

void platform_delay(unsigned int ms);
double platform_get_ms(void);
int platform_get_delta_ms(void);
void platform_delay_ms(unsigned int ms);
int platform_resource_file_path(char* path, int size, const char* filename, const char* extension);
int platform_get_asset_folder(char* path, int size);

static inline int platform_uppercase(int ascii) {
	if (ascii >= 'a' && ascii <= 'z') {
		ascii = ascii + 'A' - 'a';
	}

	return ascii;
}

#ifdef __cplusplus
}
#endif