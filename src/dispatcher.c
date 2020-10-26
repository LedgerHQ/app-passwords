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

#include "dispatcher.h"
#include "types.h"
#include "globals.h"
#include "io.h"
#include "sw.h"
#include "apdu_handlers/dump_metadatas.h"
#include "apdu_handlers/load_metadatas.h"
#include "apdu_handlers/get_version.h"
#include "apdu_handlers/get_app_name.h"

int dispatch() {
    if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        return send_sw(SW_CLA_NOT_SUPPORTED);
    }

    uint8_t ins = G_io_apdu_buffer[OFFSET_INS];
    if (app_state.current_command != ins) {
        app_state.current_command = ins;
        app_state.user_approval = false;
    }

    const buf_t input = {.bytes = G_io_apdu_buffer + OFFSET_CDATA,
                         .size = G_io_apdu_buffer[OFFSET_LC]};
    uint8_t p1 = G_io_apdu_buffer[OFFSET_P1];
    uint8_t p2 = G_io_apdu_buffer[OFFSET_P2];

    switch (ins) {
        case GET_VERSION:
            return get_version(p1, p2, &input);
        case GET_APP_NAME:
            return get_app_name(p1, p2, &input);
        case DUMP_METADATAS:
            return dump_metadatas();
        case LOAD_METADATAS:
            return load_metadatas(p1, p2, &input);
        default:
            return send_sw(SW_INS_NOT_SUPPORTED);
    }
}
