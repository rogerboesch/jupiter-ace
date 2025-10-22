
#include "ret_palette.h"

#include <string.h>
#include <math.h>

ret_color ret_palette[] = {
	// Bright 0
	0x00, 0x00, 0x00, // Black
	0x00, 0x22, 0xd7, // Blue
	0xd6, 0x28, 0x16, // Red
	0xd4, 0x33, 0xc7, // Magenta
	0x00, 0xc5, 0x25, // Green
	0x00, 0xc7, 0xc9, // Cyan
	0xcc, 0xc8, 0x2a, // Yellow
	0xca, 0xca, 0xca, // White

	// Bright 1
	0x00, 0x00, 0x00, // Black
	0x00, 0x2b, 0xfb, // Blue
	0xff, 0x33, 0x1c, // Red
	0xff, 0x40, 0xfc, // Magenta
	0x00, 0xf9, 0x2f, // Green
	0x00, 0xfb, 0xfe, // Cyan
	0xff, 0xfc, 0x36, // Yellow
	0xff, 0xff, 0xff, // White
};

ret_color RETPaletteGetColor(BYTE index) {
	if (index >= RET_PALETTE_SIZE) {
		return RETPaletteGetColor(RET_COLOR_RED);
	}

	ret_color color = ret_palette[index];
	return color;
}

ret_color RETPaletteGetColorWithBrightness(BYTE index, BYTE bindex) {
	if (index >= RET_PALETTE_SIZE) {
		return RETPaletteGetColor(2);
	}

	if (bindex > 0) {
		index += RET_PALETTE_SIZE;
	}
	
	ret_color color = ret_palette[index];
	return color;
}
