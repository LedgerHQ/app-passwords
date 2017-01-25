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

#ifndef BUI_H_
#define BUI_H_

#include <stdbool.h>

#include "bui_font.h"

/*
 * Bitmaps in this library are represented as sequences of bits, each representing the color of a single pixel (a 1 bit
 * represents the foreground color, and a 0 bit represents the background color). Bitmaps are stored big-endian (first
 * bytes in the sequence appear in lower memory addresses), rows preferred, from bottom to top and from right to left.
 */

#define BUI_HORIZONTAL_ALIGN_MASK   0b000111
#define BUI_HORIZONTAL_ALIGN_LEFT   0b000001
#define BUI_HORIZONTAL_ALIGN_CENTER 0b000010
#define BUI_HORIZONTAL_ALIGN_RIGHT  0b000100
#define BUI_VERTICAL_ALIGN_MASK     0b111000
#define BUI_VERTICAL_ALIGN_TOP      0b001000
#define BUI_VERTICAL_ALIGN_CENTER   0b010000
#define BUI_VERTICAL_ALIGN_BOTTOM   0b100000

#define BUI_DECLARE_BITMAP(name) \
		extern const unsigned char bui_bitmap_ ## name ## _w; \
		extern const unsigned char bui_bitmap_ ## name ## _h; \
		extern const unsigned char bui_bitmap_ ## name ## _bitmap[];

BUI_DECLARE_BITMAP(check);
BUI_DECLARE_BITMAP(cross);
BUI_DECLARE_BITMAP(left);
BUI_DECLARE_BITMAP(right);
BUI_DECLARE_BITMAP(up);
BUI_DECLARE_BITMAP(down);
BUI_DECLARE_BITMAP(ledger_mini);
BUI_DECLARE_BITMAP(badge_cross);
BUI_DECLARE_BITMAP(badge_dashboard);

/*
 * Initialize various data structures used by the UI manager. This should always be called once before any other
 * function in this API is called. This function fills the top and bottom display buffers with the background color.
 */
void bui_init();

/*
 * Send additional data contained within the top display buffer to the MCU if available, otherwise do nothing.
 */
void bui_display();

/*
 * Indicate to the UI manager that the MCU has processed the last display status sent. This functions should only and
 * always be called after SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT is triggered.
 */
void bui_display_processed();

/*
 * Transfer the bottom display buffer onto the top display buffer. The bottom buffer is the buffer that may be drawn
 * onto by the application, and the top buffer is the one in the process of being displayed on the screen. Calling this
 * function before the top buffer is fully displayed is legal.
 */
void bui_flush();

/*
 * Determine if the UI manager is still in the process of sending the top display buffer to the MCU to be displayed.
 *
 * Returns:
 *     false if the UI manager is in the process of sending the top display buffer to the MCU to be displayed, true
 *     otherwise
 */
bool bui_display_done();

/*
 * Fill the bottom display buffer with the specified color.
 *
 * Args:
 *     color: the color with which to fill the bottom display buffer; true is the foreground color and false is the
 *            background color
 */
void bui_fill(bool color);

/*
 * Invert every pixel in the bottom display buffer.
 */
void bui_invert();

/*
 * Fill a rectangle on the bottom display buffer. Any part of the rectangle out of bounds of the display will not be
 * drawn. If the width or height is 0, this function has no side effects.
 *
 * Args:
 *     x: the x-coordinate of top-left corner of the destination rectangle; must be >= -32,768 and <= 32,767
 *     y: the y-coordinate of top-left corner of the destination rectangle; must be >= -32,768 and <= 32,767
 *     w: the width of the rectangle; must be >= 0 and <= 32,767
 *     h: the height of the rectangle; must be >= 0 and <= 32,767
 *     color: the color with which to fill the rectangle; true is the foreground color and false is the background color
 */
void bui_fill_rect(int x, int y, int w, int h, bool color);

/*
 * Set the color of a single pixel on the bottom display buffer. If the coordinates of the pixel are out of bounds of
 * the display, this function has no side effects.
 *
 * Args:
 *     x: the x-coordinate of the pixel to be set
 *     y: the y-coordinate of the pixel to be set
 *     color: the color to which the pixel is to be set; true is the foreground color and false is the background color
 */
void bui_set_pixel(int x, int y, bool color);

/*
 * Draw a bitmap onto the bottom display buffer given a source rectangle on the bitmap's coordinate plane and a
 * destination rectangle on the display's coordinate plane. Any part of the destination rectangle out of bounds of the
 * display will not be drawn. The source rectangle must be entirely within the source bitmap. If the width or height is
 * 0, this function has no side effects.
 *
 * Args:
 *     bitmap: the pointer to the bitmap to be drawn; for each pixel, true is the foreground color and false is the
 *             background color
 *     bitmap_w: the number of pixels in a single row in the bitmap; must be >= 0 and <= 32,767
 *     src_x: the x-coordinate of the top-left corner of the source rectangle within the source bitmap's coordinate
 *            plane; must be >= 0 and <= 32,767
 *     src_y: the y-coordinate of the top-left corner of the source rectangle within the source bitmap's coordinate
 *            plane; must be >= 0 and <= 32,767
 *     dest_x: the x-coordinate of the top-left corner of the destination rectangle on or outside of the display's
 *             coordinate plane; must be >= -32,768 and <= 32,767
 *     dest_y: the y-coordinate of the top-left corner of the destination rectangle on or outside of the display's
 *             coordinate plane; must be >= -32,768 and <= 32,767
 *     w: the width of the source and destination rectangles; must be >= 0 and <= 32,767
 *     h: the height of the source and destination rectangles; must be >= 0 and <= 32,767
 */
void bui_draw_bitmap(const unsigned char *bitmap, int bitmap_w, int src_x, int src_y, int dest_x, int dest_y, int w,
		int h);

/*
 * Draw a character in the specified font onto the bottom display buffer. Any part of the character out of bounds of the
 * display will not be drawn. The coordinates provided determine the position of the text anchor. The actual bounds the
 * text is drawn within are determined by the anchor and the alignment. The alignment determines where, in the text's
 * bounds, the anchor is located. For example, a horizontal alignment of "right" and a vertical alignment of "top" will
 * place the anchor at the top-most, right-most position of the text's boundaries. Note that for purposes of alignment,
 * the character's boundaries are considered to extend from the font's baseline to the ascender height.
 *
 * Args:
 *     ch: the character code of the character to be drawn
 *     x: the x-coordinate of the text anchor; must be >= -32,768 and <= 32,767
 *     y: the y-coordinate of the text anchor; must be >= -32,768 and <= 32,767
 *     alignment: the alignment of the text relative to the anchor; this must be a value obtained by performing a
 *                bitwise OR between a constant of the form BUI_HORIZONTAL_ALIGN_<LEFT|CENTER|RIGHT> and a constant of
 *                the form BUI_VERTICAL_ALIGN_<TOP|CENTER|BOTTOM>
 *     font_id: the ID of the font to be used to render the character
 */
void bui_draw_char(unsigned char ch, int x, int y, unsigned char alignment, bui_font_id_e font_id);

/*
 * Draw a string in the specified font onto the bottom display buffer. Any part of the string out of bounds of the
 * buffer will not be drawn (the string will not wrap). The coordinates provided determine the position of the text
 * anchor. The actual bounds the text is drawn within are determined by the anchor and the alignment. The alignment
 * determines where, in the text's bounds, the anchor is located. For example, a horizontal alignment of "right" and a
 * vertical alignment of "top" will place the anchor at the top-most, right-most position of the text's boundaries. Note
 * that for purposes of alignment, the character's boundaries are considered to extend from the font's baseline to the
 * ascender height.
 *
 * Args:
 *     str: the null-terminated string to be drawn; may not be NULL
 *     x: the x-coordinate of the text anchor; must be >= -32,768 and <= 32,767
 *     y: the y-coordinate of the text anchor; must be >= -32,768 and <= 32,767
 *     alignment: the alignment of the text relative to the anchor; this must be a value obtained by performing a
 *                bitwise OR between a constant of the form BUI_HORIZONTAL_ALIGN_<LEFT|CENTER|RIGHT> and a constant of
 *                the form BUI_VERTICAL_ALIGN_<TOP|CENTER|BOTTOM>
 *     font_id: the ID of the font to be used to render the string
 */
void bui_draw_string(const unsigned char *str, int x, int y, unsigned char alignment, bui_font_id_e font_id);

#endif
