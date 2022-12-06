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
#include <os.h>

#include "io.h"
#include "globals.h"

#if !defined(TARGET_FATSTACKS)
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}
#endif

uint8_t io_event(__attribute__((unused)) uint8_t channel) {
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#if !defined(HAVE_NBGL)
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#endif
        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&  //
                !(U4BE(G_io_seproxyhal_spi_buffer, 3) &      //
                  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            /* fallthrough */
        default:
            UX_DEFAULT_EVENT();
            break;
        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#if !defined(HAVE_NBGL)
            UX_DISPLAYED_EVENT({});
#endif
#ifdef HAVE_NBGL
            UX_DEFAULT_EVENT();
#endif  // HAVE_NBGL            break;
        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
            break;
    }
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    return 1;
}

uint16_t io_exchange_al(uint8_t channel, uint16_t tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }

                return 0;
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }
        default:
            THROW(INVALID_PARAMETER);
    }

    return 0;
}

int recv() {
    int ret;

    switch (app_state.io.state) {
        case READY:
            app_state.io.state = RECEIVED;
            ret = io_exchange(CHANNEL_APDU, app_state.io.output_len);
            break;
        case RECEIVED:
            app_state.io.state = WAITING;
            ret = io_exchange(CHANNEL_APDU | IO_ASYNCH_REPLY, app_state.io.output_len);
            app_state.io.state = RECEIVED;
            break;
        case WAITING:
            app_state.io.state = READY;
            ret = -1;
            break;
    }

    return ret;
}

int send(const buf_t *buf, uint16_t sw) {
    int ret;

    if (buf != NULL) {
        memmove(G_io_apdu_buffer, buf->bytes, buf->size);
        app_state.io.output_len = buf->size;
        PRINTF("<= %.*H %02X%02X\n", buf->size, buf->bytes, sw >> 8, sw & 0xFF);
    } else {
        app_state.io.output_len = 0;
        PRINTF("<= %02X%02X\n", sw >> 8, sw & 0xFF);
    }

    G_io_apdu_buffer[app_state.io.output_len++] = (uint8_t)(sw >> 8);
    G_io_apdu_buffer[app_state.io.output_len++] = (uint8_t)(sw & 0xFF);

    switch (app_state.io.state) {
        case READY:
            ret = -1;
            break;
        case RECEIVED:
            app_state.io.state = READY;
            ret = 0;
            break;
        case WAITING:
            ret = io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, app_state.io.output_len);
            app_state.io.output_len = 0;
            app_state.io.state = READY;
            break;
    }

    return ret;
}

int send_sw(uint16_t sw) {
    return send(NULL, sw);
}
