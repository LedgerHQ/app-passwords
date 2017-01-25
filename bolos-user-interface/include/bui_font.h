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

#ifndef BUI_FONT_H_
#define BUI_FONT_H_

typedef enum {
	BUI_FONT_NONE = -1, // Not a real font
	BUI_FONT_COMIC_SANS_MS_20,
	BUI_FONT_LUCIDA_CONSOLE_8,
	BUI_FONT_LUCIDA_CONSOLE_15,
	BUI_FONT_OPEN_SANS_BOLD_13,
	BUI_FONT_OPEN_SANS_BOLD_21,
	BUI_FONT_OPEN_SANS_EXTRABOLD_11,
	BUI_FONT_OPEN_SANS_LIGHT_13,
	BUI_FONT_OPEN_SANS_LIGHT_14,
	BUI_FONT_OPEN_SANS_LIGHT_16,
	BUI_FONT_OPEN_SANS_LIGHT_20,
	BUI_FONT_OPEN_SANS_LIGHT_21,
	BUI_FONT_OPEN_SANS_LIGHT_32,
	BUI_FONT_OPEN_SANS_REGULAR_11,
	BUI_FONT_OPEN_SANS_SEMIBOLD_18,
	BUI_FONT_LAST // Not a real font
} bui_font_id_e;

// NOTE: Despite the font's range, they never include characters in the range 0x80 to 0x9F (both inclusive)
typedef struct {
	unsigned char char_height;
	unsigned char baseline_height;
	unsigned char char_kerning;
	unsigned char first_char; // Character code of the first character with a bitmap in this font
	unsigned char last_char; // Character code of the last character with a bitmap in this font
} bui_font_info_t;

/*
 * Get the info for a particular font given its ID.
 *
 * Args:
 *     font_id: the ID of the font
 * Returns:
 *     a pointer to a structure containing info about the specified font, or NULL if font_id is invalid
 */
const bui_font_info_t* bui_font_get_font_info(bui_font_id_e font_id);

/*
 * Get the width of a given character in the specified font.
 *
 * Args:
 *     font_id: the ID of the font
 *     ch: the character code
 * Returns:
 *     the width of the given character, in pixels; if font_id is invalid, 0 is returned
 */
unsigned char bui_font_get_char_width(bui_font_id_e font_id, unsigned char ch);

/*
 * Get the pointer to the bitmap for a character in a particular font.
 *
 * Args:
 *     font_id: the ID of the font
 *     ch: the character code
 *     w_dest: a pointer to an int in which the width of the character will be stored; if font_id is invalid, this is
 *             set to 0; if this is NULL, it is not accessed
 * Returns:
 *     the pointer to the bitmap for the specified character in the specified font or NULL if font_id is invalid
 */
const unsigned char* bui_font_get_char_bitmap(bui_font_id_e font_id, unsigned char ch, int *w_dest);

#endif
