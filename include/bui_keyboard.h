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

#ifndef BUI_KEYBOARD_H_
#define BUI_KEYBOARD_H_

#include <stdint.h>

typedef enum { APP_SIDE_LEFT, APP_SIDE_RIGHT } app_side_e;

void bui_keyboard_init();
void bui_keyboard_choose(app_side_e side);
void bui_keyboard_display();
void bui_keyboard_draw();
void bui_keyboard_event_button_push(unsigned int button_mask,
                                    unsigned int button_mask_counter);
void bui_keyboard_event_display_processed();
uint8_t bui_keyboard_get_typed_size(void);
uint8_t *bui_keyboard_get_typed(void);

#endif
