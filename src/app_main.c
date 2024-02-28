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

#include <cx.h>
#include <io.h>
#include <os.h>
#include <os_io_seproxyhal.h>
#include <lib_standard_app/offsets.h>
#include <stdbool.h>
#include <string.h>

#include <ctr_drbg.h>
#include <hid_mapping.h>
#include <password_generation.h>
#include <usbd_hid_impl.h>

#include "glyphs.h"
#include "dispatcher.h"
#include "error.h"
#include "globals.h"
#include "metadata.h"
#include "password_ui_flows.h"
#include "password_typing.h"

const internalStorage_t N_storage_real;
app_state_t app_state;
volatile unsigned int G_led_status;

void app_main() {
    int input_len = 0;

    init_storage();
    memset(&app_state, 0, sizeof(app_state));

    ui_idle();

    app_state.io.output_len = 0;
    app_state.io.state = READY;

#if defined(POPULATE)
#include "password.h"
    // removing 1 as `sizeof` will include the trailing null byte in the result (10)
    // but this app stores password without this trailing null byte.
    create_new_password("password1", sizeof("password1") - 1);
    create_new_password("password2", sizeof("password2") - 1);
    create_new_password("password3", sizeof("password3") - 1);
#endif
    for (;;) {
        BEGIN_TRY {
            TRY {
                input_len = io_recv_command();
                if (input_len == -1) {
                    return;
                }
                PRINTF("=> %.*H\n", input_len, G_io_apdu_buffer);
                if (input_len < OFFSET_CDATA ||
                    input_len - OFFSET_CDATA != G_io_apdu_buffer[OFFSET_LC]) {
                    io_send_sw(SW_WRONG_DATA_LENGTH);
                    continue;
                }
                if (dispatch() < 0) {
                    return;
                }
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                io_send_sw(e);
            }
            FINALLY {
            }
            END_TRY;
        }
    }
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}
