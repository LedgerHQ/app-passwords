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

#include "os.h"
#include "cx.h"
#include <stdbool.h>

#include "os_io_seproxyhal.h"
#include "string.h"

#include "glyphs.h"

#include "ctr_drbg.h"
#include "hid_mapping.h"
#include "password_generation.h"
#include "usbd_hid_impl.h"
#include "io.h"
#include "sw.h"

#include "password_ui_flows.h"
#include "password_typing.h"
#include "globals.h"
#include "metadata.h"
#include "dispatcher.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
const internalStorage_t N_storage_real;
app_state_t app_state;
volatile unsigned int G_led_status;

void app_init() {
    init_storage();
    memset(&app_state, 0, sizeof(app_state));
}

void app_main() {
    int input_len = 0;

    app_state.io.output_len = 0;
    app_state.io.state = READY;

#if defined(POPULATE)
#include "password.h"
    create_new_password("password1", 9);
    create_new_password("password2", 9);
    create_new_password("password3", 9);
    create_new_password("password4", 9);
    create_new_password("password5", 9);
    create_new_password("password6", 9);
    create_new_password("password7", 9);
#endif

    for (;;) {
        BEGIN_TRY {
            TRY {
                input_len = recv();

                if (input_len == -1) {
                    return;
                }

                PRINTF("=> %.*H\n", input_len, G_io_apdu_buffer);

                if (input_len < OFFSET_CDATA ||
                    input_len - OFFSET_CDATA != G_io_apdu_buffer[OFFSET_LC]) {
                    send_sw(SW_WRONG_DATA_LENGTH);
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
                send_sw(e);
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

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

#if defined(HAVE_NBGL)
    nbgl_objInit();
#elif defined(HAVE_BAGL)
    UX_INIT();
#endif

    BEGIN_TRY {
        TRY {
            io_seproxyhal_init();

            app_init();

            USB_power(0);
            USB_power(1);

            ui_idle();

            app_main();
        }
        CATCH_OTHER(e) {
        }
        FINALLY {
        }
    }
    END_TRY;

    app_exit();

    return 0;
}
