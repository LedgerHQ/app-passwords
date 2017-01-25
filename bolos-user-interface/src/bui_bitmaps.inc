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
 *	that you wrote the original software. If you use this software in a
 *	product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * NOTICE: This file contains work derived from source code originally subject
 * to the copyright of Ledger SAS and distributed under the terms of the Apache
 * License, Version 2.0 (which may be found here:
 * http://www.apache.org/licenses/LICENSE-2.0). The original work from which
 * this code was derived may be found here:
 * https://github.com/LedgerHQ/nanos-nonsecure-firmware/tree/master/bagl/src
 */

const unsigned char bui_bitmap_check_w = 8;
const unsigned char bui_bitmap_check_h = 6;
const unsigned char bui_bitmap_check_bitmap[] = {
	0x04, 0x0A, 0x11, 0x20, 0x40, 0x80,
};

const unsigned char bui_bitmap_cross_w = 7;
const unsigned char bui_bitmap_cross_h = 7;
const unsigned char bui_bitmap_cross_bitmap[] = {
	0x82, 0x88, 0xA0, 0x82, 0x88, 0xA0, 0x80,
};

const unsigned char bui_bitmap_left_w = 4;
const unsigned char bui_bitmap_left_h = 7;
const unsigned char bui_bitmap_left_bitmap[] = {
	0x84, 0x21, 0x24, 0x80,
};

const unsigned char bui_bitmap_right_w = 4;
const unsigned char bui_bitmap_right_h = 7;
const unsigned char bui_bitmap_right_bitmap[] = {
	0x12, 0x48, 0x42, 0x10,
};

const unsigned char bui_bitmap_up_w = 7;
const unsigned char bui_bitmap_up_h = 4;
const unsigned char bui_bitmap_up_bitmap[] = {
	0x82, 0x88, 0xA0, 0x80,
};

const unsigned char bui_bitmap_down_w = 7;
const unsigned char bui_bitmap_down_h = 4;
const unsigned char bui_bitmap_down_bitmap[] = {
	0x10, 0x51, 0x14, 0x10,
};

const unsigned char bui_bitmap_ledger_mini_w = 16;
const unsigned char bui_bitmap_ledger_mini_h = 16;
const unsigned char bui_bitmap_ledger_mini_bitmap[] = {
	0x33, 0xCC, 0x73, 0xCE, 0xF3, 0xCF, 0xF3, 0xCF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xCF, 0xFF, 0xCF,
	0xFF, 0xCF, 0xFF, 0xCF, 0xFF, 0xC0, 0xFF, 0xC0,
	0xFF, 0xCF, 0xFF, 0xCF, 0x7F, 0xCE, 0x3F, 0xCC,
};

const unsigned char bui_bitmap_badge_cross_w = 14;
const unsigned char bui_bitmap_badge_cross_h = 14;
const unsigned char bui_bitmap_badge_cross_bitmap[] = {
	0x07, 0x80, 0x7F, 0x83, 0xFF, 0x1D, 0xEE, 0x63,
	0x1B, 0xC0, 0xFF, 0x87, 0xFE, 0x1F, 0xF0, 0x3D,
	0x8C, 0x67, 0x7B, 0x8F, 0xFC, 0x1F, 0xE0, 0x1E,
	0x0F,
};

const unsigned char bui_bitmap_badge_dashboard_w = 14;
const unsigned char bui_bitmap_badge_dashboard_h = 14;
const unsigned char bui_bitmap_badge_dashboard_bitmap[] = {
	0x07, 0x80, 0x7F, 0x83, 0xFF, 0x1C, 0x0E, 0x60,
	0x1B, 0x9E, 0x7E, 0x79, 0xF9, 0xE7, 0xE7, 0x9D,
	0x80, 0x67, 0x03, 0x8F, 0xFC, 0x1F, 0xE0, 0x1E,
	0x0F,
};
