/*******************************************************************************
*   Password Manager application
*   (c) 2017 Ledger
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

#include "binary_keyboard.h"

#include <stdbool.h>

#include "bolos_target.h"

#include "bui.h"
#include "bui_bkb.h"

#if TARGET_ID == 0x31100002

bui_bitmap_128x32_t bkb_disp_buff;
int8_t bkb_disp_progress;

bui_bkb_bkb_t bkb_bkb;
char bkb_type_buff[BKB_TYPE_BUFF_CAP];

void bkb_init() {
    bui_bkb_init(&bkb_bkb, bui_bkb_layout_standard, sizeof(bui_bkb_layout_standard), bkb_type_buff, 0,
                 sizeof(bkb_type_buff),
#ifdef BKB_ANIMATE
                 true
#else
                 false
#endif
                );
}

void bkb_choose_left() {
    bui_bkb_choose(&bkb_bkb, BUI_DIR_LEFT);
}

void bkb_choose_right() {
    bui_bkb_choose(&bkb_bkb, BUI_DIR_RIGHT);
}

void bkb_draw() {
    bui_fill(&bkb_disp_buff, false);
    bui_bkb_draw(&bkb_bkb, &bkb_disp_buff);
    bkb_disp_progress = 0;
}

void bkb_animate(unsigned int elapsed) {
#ifdef BKB_ANIMATE
    if (elapsed % 10 >= 5)
        elapsed = (elapsed / 10) + 1;
    else
        elapsed /= 10;
    if (bui_bkb_tick(&bkb_bkb, elapsed)) {
        if (bkb_disp_progress == -2) {
            bkb_draw();
            bkb_display_ready();
        } else {
            bkb_draw();
        }
    }
#endif
}

void bkb_display_ready() {
    if (bkb_disp_progress == -1)
        bkb_disp_progress = -2;
    else
        bkb_disp_progress = bui_display(&bkb_disp_buff, bkb_disp_progress);
}

unsigned int bkb_get_type_buff_size() {
    return bui_bkb_get_type_buff_size(&bkb_bkb);
}

#endif
