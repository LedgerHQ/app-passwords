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

#include "bui.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "bui_font.h"

#include "bui_bitmaps.inc"

typedef struct bui_buffer_bottom_t {
	// A 128x32 bitmap. Every 128 bits is a row ordered from bottom to top, each row containing 128 pixels ordered from
	// right to left on the screen. The foreground color is represented by 1 bits, and the background color by 0 bits.
	uint8_t bitmap[512];
} bui_buffer_bottom_t;

typedef struct bui_buffer_top_t {
	// An array of 8 128x4 bitmaps, each representing a section on the screen ordered from top to bottom. Within each
	// bitmap, every 128 bits is a row ordered from bottom to top, each row containing 128 pixels ordered from right to
	// left on the screen. The foreground color is represented by 1 bits, and the background color by 0 bits.
	uint8_t bitmaps[8][64];
} bui_buffer_top_t;

// The buffer currently being displayed
static bui_buffer_top_t bui_buffer_top;
// The buffer currently available to be drawn onto by the application
static bui_buffer_bottom_t bui_buffer_bottom;
// The current progress displaying the top buffer. Odd values indicate the app is waiting for
// SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT, whereas even values indicate the app is ready to send a display status to the
// MCU. If even, this value / 2 is the index of the next bitmap to be sent. A value of 16 indicates the process is
// complete (or not ready to begin).
static int8_t bui_display_progress;

/*
 * Get a 32 bit integer storing some or all of the data for a single row of a bitmap.
 *
 * Args:
 *     bitmap: the pointer to the bitmap from which to get a row
 *     w: the width, in number of bits, of a single row in the bitmap; must be >= 1
 *     n: the number of the row from which to retrieve data from the bitmap
 *     o: an offset value in number of bits; must be < w
 * Returns:
 *     a 32 bit integer containing the sequence of bits starting at bit number n * w + o in the bitmap,
 *     truncated to w or 32 bits, whichever is lesser; the bit sequence begins at the most significant bit in the int
 */
static uint32_t bui_get_bitmap_row_32(const uint8_t *bitmap, uint32_t w, uint32_t n, uint32_t o) {
	uint64_t row = 0;
	uint32_t fi = n * w + o; // The index of the first bit to retrieve within the entire bitmap
	w -= o;
	uint32_t li = fi + (w <= 32 ? w : 32); // The index after the last bit to retrieve within the entire bitmap
	uint8_t fb = fi % 8; // The index of the first bit to retrieve within a byte
	uint8_t lb = li % 8; // The index after the last bit to retrieve within a byte
	for (uint32_t i = fi / 8; i <= (li - 1) / 8; i++) {
		uint8_t b = bitmap[i];
		// Truncate the current byte, if necessary
		if (i == li / 8 && lb != 0) {
			b >>= 8 - lb;
			b <<= 8 - lb;
		}
		row |= (uint64_t) b << ((4 + (fi / 8) - i) * 8);
	}
	row >>= 8 - fb;
	return (uint32_t) row;
}

/*
 * Shift the bits in a 128-bit array to the right by the specified number of bits.
 *
 * Args:
 *     arr: the pointer to the array of bits which is a sequence of four 32-bit ints, big-endian
 *     shift: the number of bits by which to shift to the right
 */
static void bui_rshift_128(uint32_t *arr, uint8_t shift) {
	if (shift <= 0 || shift >= 128) {
		*arr++ = 0;
		*arr++ = 0;
		*arr++ = 0;
		*arr = 0;
		return;
	}
	if (shift >= 96) {
		arr[3] = arr[0];
		arr[2] = 0;
		arr[1] = 0;
		arr[0] = 0;
		shift -= 96;
	} else if (shift >= 64) {
		arr[3] = arr[1];
		arr[2] = arr[0];
		arr[1] = 0;
		arr[0] = 0;
		shift -= 64;
	} else if (shift >= 32) {
		arr[3] = arr[2];
		arr[2] = arr[1];
		arr[1] = arr[0];
		arr[0] = 0;
		shift -= 32;
	}
	if (shift == 0)
		return;
	uint32_t mask = (1 << shift) - 1;
	arr[3] >>= shift;
	arr[3] |= (arr[2] & mask) << (32 - shift);
	arr[2] >>= shift;
	arr[2] |= (arr[1] & mask) << (32 - shift);
	arr[1] >>= shift;
	arr[1] |= (arr[0] & mask) << (32 - shift);
	arr[0] >>= shift;
}

void bui_init() {
	os_memset(&bui_buffer_top.bitmaps, 0, sizeof(bui_buffer_top.bitmaps));
	os_memset(&bui_buffer_bottom.bitmap, 0, sizeof(bui_buffer_bottom.bitmap));
	bui_display_progress = 16;
}

void bui_display() {
	if (bui_display_progress != 16 && bui_display_progress % 2 == 0) {
		uint8_t i = bui_display_progress / 2;
		unsigned int color_index[] = {0x00000000, 0x00FFFFFF};
		io_seproxyhal_display_bitmap(0, i * 4, 128, 4, color_index, 1, bui_buffer_top.bitmaps[i]);
		bui_display_progress++;
	}
}

void bui_display_processed() {
	bui_display_progress++;
}

void bui_flush() {
	// Reset display progress counter
	if (bui_display_progress % 2 == 0)
		bui_display_progress = 0;
	else
		bui_display_progress = -1;

	// Transfer the single bitmap stored in the bottom display buffer into the top display buffer, splitting it up into
	// 8 separate bitmaps and reversing the byte order in the process (this is the format required by the MCU).
	for (int i = 0; i < 512; i++)
		bui_buffer_top.bitmaps[i / 64][i % 64] = bui_buffer_bottom.bitmap[511 - i];
}

bool bui_display_done() {
	return bui_display_progress == 16;
}

void bui_fill(bool color) {
	os_memset(bui_buffer_bottom.bitmap, color ? 0xFF : 0x00, sizeof(bui_buffer_bottom.bitmap));
}

void bui_invert() {
	uint32_t *end = (uint32_t*) &bui_buffer_bottom.bitmap[512];
	for (uint32_t *ptr = (uint32_t*) &bui_buffer_bottom.bitmap[0]; ptr != end; ptr++) {
		*ptr = ~(*ptr);
	}
}

void bui_fill_rect(int x, int y, int w, int h, bool color) {
	if (x >= 128 || y >= 32 || w == 0 || h == 0)
		return;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x + w > 128)
		w = 128 - x;
	if (y + h > 32)
		h = 32 - y;
	// Reflect coordinates
	x = 128 - x - w;
	y = 32 - y - h;
	w += x; // w is now the x-coord just beyond the bottom right corner
	h += y; // h is now the y-coord just beyond the bottom right corner
	for (int i = y; i < h; i++) {
		uint8_t *row = &bui_buffer_bottom.bitmap[i * 16];
		for (int j = 32; j != 160; j += 32) { // Iterate through all four 32 bit sequences in the row
			if (x < j && w > j - 32) { // Only need to blit if current long is within rectangle
				uint32_t mask = 0xFFFFFFFF; // The mask used to blit onto the current long
				if (x > j - 32) { // If some bits need to be shaved off the start of the mask
					mask >>= x % 32;
				}
				if (w < j) { // If some bits need to be shaved off the end of the mask
					unsigned int s = j - w;
					mask >>= s;
					mask <<= s;
				}
				// Blit the mask onto part of the row
				if (color) {
					*row++ |= (uint8_t) (mask >> 24);
					*row++ |= (uint8_t) (mask >> 16);
					*row++ |= (uint8_t) (mask >> 8);
					*row++ |= (uint8_t) mask;
				} else {
					*row++ &= ~((uint8_t) (mask >> 24));
					*row++ &= ~((uint8_t) (mask >> 16));
					*row++ &= ~((uint8_t) (mask >> 8));
					*row++ &= ~((uint8_t) mask);
				}
			} else {
				row += 4;
			}
		}
	}
}

void bui_set_pixel(int x, int y, bool color) {
	if (x < 0 || x >= 128 || y < 0 || y >= 32)
		return;
	// Reflect coordinates
	x = 128 - x;
	y = 32 - y;
	// Find destination
	uint32_t dest_bit = y * 128 + x;
	uint32_t dest_byte = dest_bit / 8;
	dest_bit %= 8;
	// Blit
	if (color)
		bui_buffer_bottom.bitmap[dest_byte] |= 0x80 >> dest_bit;
	else
		bui_buffer_bottom.bitmap[dest_byte] &= ~(0x80 >> dest_bit);
}

void bui_draw_bitmap(const unsigned char *bitmap, int bitmap_w, int src_x, int src_y, int dest_x, int dest_y, int w,
		int h) {
	if (dest_x >= 128 || dest_y >= 32 || w == 0 || h == 0 || dest_x + w <= 0 || dest_y + h <= 0)
		return;
	// Shift source and destination coordinates such that the destination rectangle is entirely contained in the display
	if (dest_x < 0) {
		src_x -= dest_x;
		w -= dest_x;
		dest_x = 0;
	}
	if (dest_y < 0) {
		src_y -= dest_y;
		h -= dest_y;
		dest_y = 0;
	}
	// Truncate width and height to fit within screen
	if (dest_x + w > 128)
		w = 128 - dest_x;
	if (dest_y + h > 32)
		h = 32 - dest_y;
	// Reflect coordinates
	dest_x = 128 - dest_x - w;
	dest_y = 32 - dest_y - h;
	w += dest_x; // w is now the x-coord just beyond the bottom right corner, reflected
	h += dest_y; // h is now the y-coord just beyond the bottom right corner, reflected
	for (int i = dest_y; i < h; i++) {
		// Load data from the bitmap for the current row into a buffer and align it
		uint32_t source_row[4] = {0, 0, 0, 0};
		for (uint8_t j = 0; j <= (w - 1) / 32; j++) {
			source_row[j] = bui_get_bitmap_row_32(bitmap, bitmap_w, src_y + i - dest_y, src_x + j * 32);
		}
		bui_rshift_128(source_row, dest_x);
		uint8_t *dest_row = &bui_buffer_bottom.bitmap[i * 16];
		for (int j = 32; j != 160; j += 32) { // Iterate through all four 32 bit sequences in the row
			if (dest_x < j && w > j - 32) { // Only need to blit if current long is within rectangle
				uint32_t mask = 0xFFFFFFFF; // The mask used to blit onto the current long
				if (dest_x > j - 32) { // If some bits need to be shaved off the start of the mask
					mask >>= dest_x % 32;
				}
				if (w < j) { // If some bits need to be shaved off the end of the mask
					int s = j - w;
					mask >>= s;
					mask <<= s;
				}
				// Blit part of the source row onto part of the destination row
				dest_row += 3;
				*dest_row = (*dest_row & ~((uint8_t) mask)) | ((uint8_t) source_row[j / 32 - 1] & (uint8_t) mask);
				dest_row--;
				mask >>= 8;
				source_row[j / 32 - 1] >>= 8;
				*dest_row = (*dest_row & ~((uint8_t) mask)) | ((uint8_t) source_row[j / 32 - 1] & (uint8_t) mask);
				dest_row--;
				mask >>= 8;
				source_row[j / 32 - 1] >>= 8;
				*dest_row = (*dest_row & ~((uint8_t) mask)) | ((uint8_t) source_row[j / 32 - 1] & (uint8_t) mask);
				dest_row--;
				mask >>= 8;
				source_row[j / 32 - 1] >>= 8;
				*dest_row = (*dest_row & ~((uint8_t) mask)) | ((uint8_t) source_row[j / 32 - 1] & (uint8_t) mask);
				dest_row += 4;
			} else {
				dest_row += 4;
			}
		}
	}
}

void bui_draw_char(unsigned char ch, int x, int y, unsigned char alignment, bui_font_id_e font_id) {
	if (x >= 128 || y >= 32)
		return;
	const bui_font_info_t *font_info = bui_font_get_font_info(font_id);
	int h = font_info->char_height;
	int w;
	const unsigned char *bitmap = bui_font_get_char_bitmap(font_id, ch, &w);
	switch (alignment & BUI_HORIZONTAL_ALIGN_MASK) {
	case BUI_HORIZONTAL_ALIGN_LEFT:
		break;
	case BUI_HORIZONTAL_ALIGN_CENTER:
		x -= w / 2;
		if (w % 2 == 1)
			x -= 1;
		break;
	case BUI_HORIZONTAL_ALIGN_RIGHT:
		x -= w;
		break;
	}
	switch (alignment & BUI_VERTICAL_ALIGN_MASK) {
	case BUI_VERTICAL_ALIGN_TOP:
		break;
	case BUI_VERTICAL_ALIGN_CENTER:
		y -= h / 2;
		if (h % 2 == 1)
			y -= 1;
		break;
	case BUI_VERTICAL_ALIGN_BOTTOM:
		y -= h;
		break;
	}
	bui_draw_bitmap(bitmap, w, 0, 0, x, y, w, h);
}

void bui_draw_string(const unsigned char *str, int x, int y, unsigned char alignment, bui_font_id_e font_id) {
	const bui_font_info_t *font_info = bui_font_get_font_info(font_id);
	switch (alignment & BUI_VERTICAL_ALIGN_MASK) {
	case BUI_VERTICAL_ALIGN_TOP:
		break;
	case BUI_VERTICAL_ALIGN_CENTER:
		y -= font_info->baseline_height / 2;
		if (font_info->baseline_height % 2 == 1)
			y -= 1;
		break;
	case BUI_VERTICAL_ALIGN_BOTTOM:
		y -= font_info->baseline_height;
		break;
	}
	if (y >= 32 || y + font_info->char_height <= 0)
		return;
	if ((alignment & BUI_HORIZONTAL_ALIGN_MASK) != BUI_HORIZONTAL_ALIGN_LEFT) {
		int w = 0;
		for (const unsigned char *s = str; *s != '\0'; s++) {
			w += bui_font_get_char_width(font_id, *s);
			w += font_info->char_kerning;
		}
		if ((alignment & BUI_HORIZONTAL_ALIGN_MASK) == BUI_HORIZONTAL_ALIGN_CENTER) {
			x -= w / 2;
			if (w % 2 == 1)
				x -= 1;
		} else {
			x -= w;
		}
		if (x + w <= 0)
			return;
	}
	if (x >= 128)
		return;
	for (; *str != '\0' && x < 128; str++) {
		int w;
		const unsigned char *bitmap = bui_font_get_char_bitmap(font_id, *str, &w);
		bui_draw_bitmap(bitmap, w, 0, 0, x, y, w, font_info->char_height);
		x += w;
		x += font_info->char_kerning;
	}
}
