/*
 * License for the BOLOS User Interface project, originally found here:
 * https://github.com/parkerhoyes/bolos-user-interface
 *
 * Copyright (C) 2016 Parker Hoyes <contact@parkerhoyes.com>
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 *    that you wrote the original software. If you use this software in a
 *    product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "bui_font.h"

#include <stdint.h>
#include <stddef.h>

#include "os.h"

typedef struct {
	uint8_t char_width; // Character width, in pixels
	uint16_t bitmap_offset; // The starting index of the character's bitmap in the font bitmap
} bui_font_char_t;

// NOTE: Despite the font's range, they never include characters in the range 0x80 to 0x9F (both inclusive)
typedef struct {
	unsigned char char_height;
	unsigned char baseline_height;
	unsigned char char_kerning;
	unsigned char first_char; // Character code of the first character in the font bitmap
	unsigned char last_char; // Character code of the last character in the font bitmap
	const bui_font_char_t *chars;
	const unsigned char *bitmaps; // Array of bitmaps for all characters
} bui_font_data_t;

typedef union {
	bui_font_data_t data;
	bui_font_info_t info;
} bui_font_t;

#include "bui_font_fonts.inc"

const bui_font_info_t* bui_font_get_font_info(bui_font_id_e font_id) {
	if ((int) font_id <= BUI_FONT_NONE || (int) font_id >= BUI_FONT_LAST)
		return NULL;
	return &((const bui_font_t*) PIC(bui_fonts[(int) font_id]))->info;
}

unsigned char bui_font_get_char_width(bui_font_id_e font_id, unsigned char ch) {
	if ((int) font_id <= BUI_FONT_NONE || (int) font_id >= BUI_FONT_LAST)
		return 0;
	const bui_font_data_t *font_data = &((const bui_font_t*) PIC(bui_fonts[(int) font_id]))->data;
	uint8_t chari = ch;
	if (chari >= 0x80)
		chari -= 0xA0 - 0x80;
	chari -= font_data->first_char;
	return ((const bui_font_char_t*) PIC(font_data->chars))[chari].char_width;
}

const unsigned char* bui_font_get_char_bitmap(bui_font_id_e font_id, unsigned char ch, int *w_dest) {
	if ((int) font_id <= BUI_FONT_NONE || (int) font_id >= BUI_FONT_LAST) {
		if (w_dest != NULL)
			*w_dest = 0;
		return NULL;
	}
	const bui_font_data_t *font_data = &((const bui_font_t*) PIC(bui_fonts[(int) font_id]))->data;
	uint8_t chari = ch;
	if (chari >= 0x80)
		chari -= 0xA0 - 0x80;
	chari -= font_data->first_char;
	bui_font_char_t font_char = ((const bui_font_char_t*) PIC(font_data->chars))[chari];
	if (w_dest != NULL)
		*w_dest = font_char.char_width;
	return (const unsigned char*) PIC(font_data->bitmaps) + font_char.bitmap_offset;
}
