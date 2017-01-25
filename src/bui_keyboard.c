/*
 * License for the Ledger Nano S Binary Keyboard Demo project, originally found
 * here: https://github.com/parkerhoyes/nanos-app-binarykbdemo
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

#include "bui_keyboard.h"

#include <stdbool.h>
#include <stdint.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "bui.h"

#define CEIL_DIV(x, y) (1 + (((x)-1) / (y)))
#define NTH_BIT(n, i) (((n) >> (7 - (i))) & 1)

// The buffer that stores the characters that the user has typed
static uint8_t app_type_buff[19];
// The number of characters in app_type_buff
static uint8_t app_type_buff_size;
// The buffer that stores the possible characters the user may choose from (the
// order matters)
static uint8_t app_chars[36];
// The number of characters in app_chars
static uint8_t app_chars_size;

// The sequence of "bits" inputted by the user, starting at the MSB (0 is left,
// 1 is right)
static uint8_t app_bits_typed;
// The number of "bits" inputted by the user (the number of left / right
// choices)
static uint8_t app_bits_typed_size;

void bui_keyboard_init() {
    app_type_buff_size = 0;
    for (uint8_t i = 0; i < 26; i++) {
        app_chars[i] = 'A' + i;
    }
    app_chars[26] = '_';
    app_chars[27] = '<';
    app_chars_size = 27;
    app_bits_typed = 0;
    app_bits_typed_size = 0;

    bui_keyboard_display();
}

void bui_keyboard_choose(app_side_e side) {
    if (side == APP_SIDE_RIGHT)
        app_bits_typed |= 0x80 >> app_bits_typed_size;
    app_bits_typed_size += 1;
    uint8_t charsn = app_chars_size;
    uint8_t charsi = 0;
    for (uint8_t i = 0; i < app_bits_typed_size; i++) {
        if (NTH_BIT(app_bits_typed, i) == 0) {
            charsn = CEIL_DIV(charsn, 2);
        } else {
            charsi += CEIL_DIV(charsn, 2);
            charsn /= 2;
        }
    }
    if (charsn == 1) {
        uint8_t ch = app_chars[charsi];
        switch (ch) {
        case '<':
            app_type_buff_size -= 1;
            break;
        case '_':
            ch = ' ';
        default:
            app_type_buff[app_type_buff_size++] = ch;
            break;
        }
        app_bits_typed = 0;
        app_bits_typed_size = 0;
        if (app_type_buff_size == 0) {
            app_chars_size = 27;
            app_chars[0] = 'A';
            app_chars[1] = 'B';
        } else if (app_type_buff_size == 19) {
            app_chars_size = 2;
            app_chars[0] = '<';
            app_chars[1] = '<';
        } else {
            app_chars_size = 28;
            app_chars[0] = 'A';
            app_chars[1] = 'B';
        }
    }
}

void bui_keyboard_display() {
    bui_fill(false);
    bui_keyboard_draw();
    bui_flush();
    bui_display();
}

void bui_keyboard_draw() {
    // Draw textbox border
    bui_fill_rect(1 - 1, 20 - 1, 126, 1,
                  true); // These coords aren't right, are they?
    bui_set_pixel(1, 21, true);
    bui_set_pixel(126, 21, true);
    bui_fill_rect(1 - 1, 31 - 1, 126, 1,
                  true); // These coords aren't right, are they?
    bui_set_pixel(1, 30, true);
    bui_set_pixel(126, 30, true);

    // Draw textbox contents
    for (int i = 0; i < app_type_buff_size; i++) {
        bui_draw_char(app_type_buff[i], i * 6 + 3, 22,
                      BUI_HORIZONTAL_ALIGN_LEFT | BUI_VERTICAL_ALIGN_TOP,
                      BUI_FONT_LUCIDA_CONSOLE_8);
    }
    bui_fill_rect(app_type_buff_size * 6 + 3, 28, 5, 1, true);

    // Draw center icons
    bui_draw_bitmap(bui_bitmap_left_bitmap, bui_bitmap_left_w, 0, 0, 58, 5,
                    bui_bitmap_left_w, bui_bitmap_left_h);
    bui_draw_bitmap(bui_bitmap_right_bitmap, bui_bitmap_right_w, 0, 0, 66, 5,
                    bui_bitmap_right_w, bui_bitmap_right_h);

    // Draw keyboard "keys"
    uint8_t charsi = 0;
    uint8_t charsn = app_chars_size;
    for (uint8_t i = 0; i < app_bits_typed_size; i++) {
        if (NTH_BIT(app_bits_typed, i) == 0) {
            charsn = CEIL_DIV(charsn, 2);
        } else {
            charsi += CEIL_DIV(charsn, 2);
            charsn /= 2;
        }
    }
    uint8_t lefti = charsi;
    uint8_t leftn = CEIL_DIV(charsn, 2);
    for (uint8_t i = 0; i < leftn; i++) {
        bui_draw_char(app_chars[lefti + i], 1 + 6 * (i % 9), i < 9 ? 0 : 9,
                      BUI_HORIZONTAL_ALIGN_LEFT | BUI_VERTICAL_ALIGN_TOP,
                      BUI_FONT_LUCIDA_CONSOLE_8);
    }
    uint8_t righti = lefti + leftn;
    uint8_t rightn = charsn / 2;
    for (uint8_t i = 0; i < rightn; i++) {
        bui_draw_char(app_chars[righti + i], 74 + 6 * (i % 9), i < 9 ? 0 : 9,
                      BUI_HORIZONTAL_ALIGN_LEFT | BUI_VERTICAL_ALIGN_TOP,
                      BUI_FONT_LUCIDA_CONSOLE_8);
    }
}

void bui_keyboard_event_button_push(unsigned int button_mask,
                                    unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        os_sched_exit(0); // Go back to the dashboard
        break;
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        bui_keyboard_choose(APP_SIDE_LEFT);
        bui_keyboard_display();
        break;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        bui_keyboard_choose(APP_SIDE_RIGHT);
        bui_keyboard_display();
        break;
    }
}

void bui_keyboard_event_display_processed() {
    bui_display_processed();
    bui_display();
}

uint8_t bui_keyboard_get_typed_size() {
    return app_type_buff_size;
}

uint8_t *bui_keyboard_get_typed() {
    return app_type_buff;
}