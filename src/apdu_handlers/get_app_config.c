/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
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
 *****************************************************************************/

#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "../password_ui_flows.h"
#include "get_app_config.h"
#include "io.h"
#include "sw.h"
#include "types.h"
#include "globals.h"

int get_app_config(uint8_t p1, uint8_t p2, __attribute__((unused)) const buf_t* input) {
    if (p1 != 0 || p2 != 0) {
        return send_sw(SW_WRONG_P1P2);
    }

    uint8_t* config = G_io_apdu_buffer;
    size_t offset = 0;

    config[offset++] = ((size_t) MAX_METADATAS) >> 8 * 3;
    config[offset++] = ((size_t) MAX_METADATAS) >> 8 * 2;
    config[offset++] = ((size_t) MAX_METADATAS) >> 8 * 1;
    config[offset++] = ((size_t) MAX_METADATAS) & 0xFF;

    config[offset++] = N_storage.keyboard_layout;
    config[offset++] = N_storage.press_enter_after_typing;

    const buf_t buf = {.bytes = config, .size = offset};

    ui_idle();
    return send(&buf, SW_OK);
}
