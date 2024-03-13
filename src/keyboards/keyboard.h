/*******************************************************************************
 *   Password Manager application
 *   (c) 2017-2023 Ledger SAS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

#if !defined(SCREEN_SIZE_WALLET)

#include <os_io_seproxyhal.h>
#include <ux.h>

#define KEYBOARD_ITEM_VALIDATED \
    1  // callback is called with the entered item index, tmp_element is
       // precharged with element to be displayed and using the common string
       // buffer as string parameter
#define KEYBOARD_RENDER_ITEM \
    2  // callback is called the element index, tmp_element is precharged with
       // element to be displayed and using the common string buffer as string
       // parameter
#define KEYBOARD_RENDER_WORD \
    3  // callback is called with a -1 when requesting complete word, or the char
       // index else, returnin 0 implies no char is to be displayed
typedef const bagl_element_t* (*keyboard_callback_t)(unsigned int event, unsigned int value);

// bolos ux context (not mandatory if redesigning a bolos ux)
typedef struct keyboard_ctx {
    bagl_element_t tmp_element;

    unsigned int onboarding_step;

    unsigned int words_buffer_length;
    // after an int to make sure it's aligned
    char string_buffer[10];

    char words_buffer[25];

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    char title[20];
#endif

    // slider management
    unsigned int hslider3_before;
    unsigned int hslider3_current;
    unsigned int hslider3_after;
    unsigned int hslider3_total;

    keyboard_callback_t keyboard_callback;

} keyboard_ctx_t;

extern keyboard_ctx_t G_keyboard_ctx;

// update before, current, after index for horizontal slider with 3 positions
// slider distinguish handling from the data, to be more generic :)
#define BOLOS_UX_HSLIDER3_NONE (-1UL)
void bolos_ux_hslider3_init(unsigned int total_count);
void bolos_ux_hslider3_set_current(unsigned int current);
void bolos_ux_hslider3_next(void);
void bolos_ux_hslider3_previous(void);

#define COMMON_KEYBOARD_INDEX_UNCHANGED (-1UL)

void screen_common_keyboard_init(unsigned int stack_slot,
                                 unsigned int current_element,
                                 unsigned int nb_elements,
                                 keyboard_callback_t callback);
void screen_text_keyboard_init(char* buffer, unsigned int maxsize, appmain_t validation_callback);

#endif
