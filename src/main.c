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

#include "os.h"
#include "cx.h"
#include <stdbool.h>

#include "os_io_seproxyhal.h"
#include "string.h"

#include "glyphs.h"

#include "binary_keyboard.h"
#include "ctr_drbg.h"
#include "hid_mapping.h"
#include "password_generation.h"

#define META_NONE 0x00
#define META_ERASED 0xFF

#define DERIVE_PASSWORD_PATH 0x80505744

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define CLA 0xE0

bagl_element_t tmp_element;

ux_state_t ux;
// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;

uint32_t currentMetadataOffset;
uint8_t firstMetadataFound;
uint8_t metaName[MAX_METANAME];
enum {
    MODE_NONE,
    MODE_CREATE,
    MODE_TYPE,
    MODE_REMOVE,
};
uint8_t mode;

typedef struct internalStorage_t {
#define STORAGE_MAGIC 0xDEAD1337
    uint32_t magic;
    uint32_t keyboard_layout;
    uint8_t metadatas[MAX_METADATAS];
} internalStorage_t;

WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))

uint8_t write_metadata(uint8_t *data, uint8_t dataSize);

static const uint8_t EMPTY_REPORT[] = {0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00};
static const uint8_t SPACE_REPORT[] = {0x00, 0x00, 0x2C, 0x00,
                                       0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_REPORT[] = {0x02, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_LOCK_REPORT[] = {0x00, 0x00, 0x39, 0x00,
                                           0x00, 0x00, 0x00, 0x00};
static const uint8_t ENTER_REPORT[] = {0x00, 0x00, 0x28, 0x00,
                                       0x00, 0x00, 0x00, 0x00};

volatile unsigned int G_led_status;

static const uint8_t DEFAULT_MIN_SET[] = {1, 1, 1, 0, 0, 1, 0, 0};

uint8_t entropyProvided;
uint8_t entropy[32];

int entropyProvider2(void *context, unsigned char *buffer, size_t bufferSize) {
    if (entropyProvided) {
        return 1;
    }
    memcpy(buffer, entropy, 32);
    entropyProvided = 1;
    return 0;
}

void type_password(uint8_t *data, uint32_t dataSize, uint8_t *out,
                   setmask_t setMask, const uint8_t *minFromSet,
                   uint32_t size) {
    uint32_t derive[9];
    uint32_t led_status;
    uint8_t tmp[64];
    uint8_t i;
    uint8_t report[8];
    cx_hash_sha256(data, dataSize, tmp);
    derive[0] = DERIVE_PASSWORD_PATH;
    for (i = 0; i < 8; i++) {
        derive[i + 1] = (tmp[4 * i] << 24) | (tmp[4 * i + 1] << 16) |
                        (tmp[4 * i + 2] << 8) | (tmp[4 * i + 3]);
        derive[i + 1] |= 0x80000000;
    }
    os_perso_derive_node_bip32(CX_CURVE_SECP256K1, derive, 9, tmp, tmp + 32);
    cx_hash_sha256(tmp, 64, entropy);
    os_memset(tmp, 0, sizeof(tmp));
    entropyProvided = 0;
    mbedtls_ctr_drbg_context ctx;
    mbedtls_ctr_drbg_init(&ctx);
    if (mbedtls_ctr_drbg_seed(&ctx, entropyProvider2, NULL, NULL, 0) != 0) {
        THROW(EXCEPTION);
    }
    if (out == NULL) {
        out = tmp;
    }

    generate_password(&ctx, setMask, minFromSet, out, size);
#if 0
    out = "A-B_C D\\E\"F#G$H%I&J'K*L+M,N.O/P:Q;R=S?T@U^V`W|X~Y[Z]a{b}c(d)e<f>g!hijklmnopqrstuvwxyz0123456789~e^e'e`e\"e\"\"e";
    size = strlen(out);
#endif

    os_memset(report, 0, sizeof(report));
    // Insert EMPTY_REPORT CAPS_REPORT EMPTY_REPORT to avoid undesired capital
    // letter on KONSOLE
    led_status = G_led_status;
    io_usb_send_ep(1, EMPTY_REPORT, 8, 20);
    io_usb_send_ep(1, CAPS_REPORT, 8, 20);
    io_usb_send_ep(1, EMPTY_REPORT, 8, 20);

    // toggle shift if set.
    if (led_status & 2) {
        io_usb_send_ep(1, CAPS_LOCK_REPORT, 8, 20);
        io_usb_send_ep(1, EMPTY_REPORT, 8, 20);
    }
    for (i = 0; i < size; i++) {
        // If keyboard layout not initialized, use the default
        map_char(N_storage.keyboard_layout, out[i], report);
        io_usb_send_ep(1, report, 8, 20);
        io_usb_send_ep(1, EMPTY_REPORT, 8, 20);

        if (N_storage.keyboard_layout == HID_MAPPING_QWERTY_INTL) {
            switch (out[i]) {
            case '\"':
            case '\'':
            case '`':
            case '~':
            case '^':
                // insert a extra space to validate the symbol
                io_usb_send_ep(1, SPACE_REPORT, 8, 20);
                io_usb_send_ep(1, EMPTY_REPORT, 8, 20);
                break;
            }
        }
    }
    // restore shift state
    if (led_status & 2) {
        io_usb_send_ep(1, CAPS_LOCK_REPORT, 8, 20);
        io_usb_send_ep(1, EMPTY_REPORT, 8, 20);
    }

    // press enter
    io_usb_send_ep(1, ENTER_REPORT, 8, 20);
    io_usb_send_ep(1, EMPTY_REPORT, 8, 20);
}

void reset_metadatas(void) {
    nvm_write(N_storage.metadatas, NULL, sizeof(N_storage.metadatas));
}

void erase_metadata(uint32_t offset) {
    unsigned char m = META_ERASED;
    nvm_write(&N_storage.metadatas[offset + 1], &m, 1);
}

uint32_t find_free_metadata(void) {
    uint32_t offset = 0;
    while ((N_storage.metadatas[offset] != 0) && (offset < MAX_METADATAS)) {
        offset += N_storage.metadatas[offset] + 2;
    }
    return offset;
}

uint32_t find_next_metadata(uint32_t offset) {
    if (!firstMetadataFound) {
        firstMetadataFound = (N_storage.metadatas[0] != 0);
        return 0;
    }
    offset += N_storage.metadatas[offset] + 2;
    for (;;) {
        if (N_storage.metadatas[offset] == 0) {
            return 0; // end of file
        }
        if (N_storage.metadatas[offset + 1] != META_ERASED) {
            return offset; // next entry
        }
        offset += N_storage.metadatas[offset] + 2;
    }
}

uint32_t get_metadata(uint32_t nth) {
    unsigned int offset = 0;
    for (;;) {
        if (N_storage.metadatas[offset] == 0) {
            return -1UL; // end of file
        }
        if (N_storage.metadatas[offset + 1] != META_ERASED) {
            if (nth == 0) {
                return offset;
            }
            nth--;
        }
        offset += N_storage.metadatas[offset] + 2;
    }
}

uint32_t find_previous_metadata(uint32_t offset) {
    uint32_t searchOffset = 0;
    uint32_t lastValidOffset = 0;
    if (offset == 0) {
        return 0;
    }
    for (;;) {
        if (N_storage.metadatas[searchOffset + 1] != META_ERASED) {
            lastValidOffset = searchOffset;
        }
        searchOffset += N_storage.metadatas[searchOffset] + 2;
        if (searchOffset == offset) {
            break;
        }
    }
    return lastValidOffset;
}

uint8_t write_metadata(uint8_t *data, uint8_t dataSize) {
    uint8_t tmp[2];
    uint32_t offset = find_free_metadata();
    if ((offset + dataSize + 2 + 2) > MAX_METADATAS) {
        return 0;
    }
    nvm_write(&N_storage.metadatas[offset + 2], data, dataSize);
    tmp[0] = 0;
    tmp[1] = META_NONE;
    nvm_write(&N_storage.metadatas[offset + 2 + dataSize], tmp, 2);
    tmp[0] = dataSize;
    tmp[1] = META_NONE;
    nvm_write(&N_storage.metadatas[offset], tmp, 2);
    return 1;
}

#if defined(TARGET_NANOS)

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
void menu_remove_init(unsigned int start);

// change the setting
void menu_settings_layout_change(uint32_t layout) {
    nvm_write(&N_storage.keyboard_layout, (void *)&layout, sizeof(uint32_t));
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

const ux_menu_entry_t menu_settings_layout[] = {
    {NULL, menu_settings_layout_change, HID_MAPPING_QWERTY, NULL, "Qwerty",
     NULL, 0, 0},
    {NULL, menu_settings_layout_change, HID_MAPPING_QWERTY_INTL, NULL,
     "Qwerty Int'l", NULL, 0, 0},
    {NULL, menu_settings_layout_change, HID_MAPPING_AZERTY, NULL, "Azerty",
     NULL, 0, 0},
    //  {NULL, menu_settings_layout_change, 3, NULL, "Qwertz", NULL, 0, 0},
    UX_MENU_END};

// show the currently activated entry
void menu_settings_layout_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(
        N_storage.keyboard_layout > 0 ? N_storage.keyboard_layout - 1 : 0,
        menu_settings_layout, NULL);
}

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_layout_init, 0, NULL, "Keyboard layout", NULL, 0, 0},
    {menu_main, NULL, 4, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 5, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

void menu_reset_confirm(unsigned int ignored) {
    UNUSED(ignored);
    reset_metadatas();
    UX_MENU_DISPLAY(3, menu_main, NULL);
}

const ux_menu_entry_t menu_reset_all[] = {
    {menu_main, NULL, 3, NULL, "No", NULL, 0, 0},
    {NULL, menu_reset_confirm, 0, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};

void menu_entries_type_password(void) {
    unsigned int offset = get_metadata(ux_menu.current_entry);
    // todo store the set allowed per metadate
    type_password(&N_storage.metadatas[offset + 2], N_storage.metadatas[offset],
                  NULL, ALL_SETS, (const uint8_t *)PIC(DEFAULT_MIN_SET), 20);
}

uint32_t removed_entry;
void menu_entry_reset_confirm(unsigned int ignored) {
    UNUSED(ignored);
    unsigned int offset = get_metadata(removed_entry);
    erase_metadata(offset);
    // redisplay the complete remove menu (starting where we erased the entry)
    menu_remove_init(removed_entry);
}

void menu_entry_reset_cancel(unsigned int ignored) {
    UNUSED(ignored);
    // redisplay the complete remove menu (starting where we DID NOT erased the
    // entry)
    menu_remove_init(removed_entry);
}

const ux_menu_entry_t menu_entry_reset[] = {
    {NULL, menu_entry_reset_cancel, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_entry_reset_confirm, 0, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};

void menu_entries_remove(void) {
    removed_entry = ux_menu.current_entry;
    UX_MENU_DISPLAY(0, menu_entry_reset, NULL);
}

unsigned char metaName_prev[MAX_METANAME];
unsigned char metaName_next[MAX_METANAME];
const ux_menu_entry_t menu_entries_default[] = {
    {NULL, NULL, 0, NULL, metaName_prev, NULL, 0, 0},
    {NULL, menu_entries_type_password, 0, NULL, metaName, NULL, 0, 0},
    {NULL, NULL, 0, NULL, metaName_next, NULL, 0, 0},
    {menu_main, NULL, 0, &C_icon_back, "Back", NULL, 61, 40},
};
ux_menu_entry_t fake_entries[4];

const ux_menu_entry_t *menu_entries_iterator(unsigned int entry_index) {
    unsigned int offset;

    // the last entry is "back"
    if (entry_index == ux_menu.menu_entries_count - 1) {
        os_memmove(&fake_entries[3], &menu_entries_default[3],
                   sizeof(ux_menu_entry_t));
        switch (mode) {
        case MODE_TYPE:
            fake_entries[3].userid = 0;
            break;
        case MODE_REMOVE:
            fake_entries[3].userid = 2;
            break;
        }
        return &fake_entries[3];
    }

    // get previous
    // not called if no previous element
    if (ux_menu.current_entry > entry_index) {
        os_memmove(&fake_entries[0], &menu_entries_default[0],
                   sizeof(ux_menu_entry_t));
        offset = get_metadata(entry_index);
        os_memmove(metaName_prev, &N_storage.metadatas[offset + 2],
                   N_storage.metadatas[offset]);
        metaName_prev[N_storage.metadatas[offset]] = 0;
        return &fake_entries[0];
    }
    // get current
    if (ux_menu.current_entry == entry_index) {
        os_memmove(&fake_entries[1], &menu_entries_default[1],
                   sizeof(ux_menu_entry_t));
        offset = get_metadata(entry_index);
        os_memmove(metaName, &N_storage.metadatas[offset + 2],
                   N_storage.metadatas[offset]);
        metaName[N_storage.metadatas[offset]] = 0;
        switch (mode) {
        case MODE_TYPE:
            fake_entries[1].callback = menu_entries_type_password;
            break;
        case MODE_REMOVE:
            fake_entries[1].callback = menu_entries_remove;
            break;
        }
        return &fake_entries[1];
    }
    // get next
    // not called if no next element
    if (ux_menu.current_entry < entry_index) {
        os_memmove(&fake_entries[2], &menu_entries_default[2],
                   sizeof(ux_menu_entry_t));
        offset = get_metadata(entry_index);
        os_memmove(metaName_next, &N_storage.metadatas[offset + 2],
                   N_storage.metadatas[offset]);
        metaName_next[N_storage.metadatas[offset]] = 0;
        return &fake_entries[2];
    }
}

void menu_entries_init(unsigned int start) {
    UNUSED(start);
    unsigned int offset;
    UX_MENU_DISPLAY(start, NULL, NULL);
    // count number of entries
    ux_menu.menu_entries_count = 0;
    while (get_metadata(ux_menu.menu_entries_count) != -1UL) {
        ux_menu.menu_entries_count++;
    }
    // the back item
    ux_menu.menu_entries_count++;
    // setup iterator
    ux_menu.menu_iterator = menu_entries_iterator;
    mode = MODE_TYPE;
}

void menu_remove_init(unsigned int start) {
    menu_entries_init(start);
    mode = MODE_REMOVE;
}

unsigned int handle_button_push_create_entry(unsigned int button_mask,
                                             unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        mode = MODE_NONE;
        write_metadata(bkb_type_buff, bkb_get_type_buff_size());
        UX_MENU_DISPLAY(1, menu_main, NULL);
        break;
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        bkb_choose_left();
        bkb_draw();
        bkb_display_ready();
        break;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        bkb_choose_right();
        bkb_draw();
        bkb_display_ready();
        break;
    }
    return 0;
}

void menu_new_entry(void) {
    mode = MODE_CREATE;
    bkb_init();
    bkb_draw();
    bkb_display_ready();
}

const ux_menu_entry_t menu_main[] = {
    {NULL, menu_entries_init, 0, NULL, "Type password", NULL, 0, 0},
    {NULL, menu_new_entry, 0, NULL, "New password", NULL, 0, 0},
    {NULL, menu_remove_init, 0, NULL, "Delete password", NULL, 0, 0},
    {menu_reset_all, NULL, 0, NULL, "Reset all", NULL, 0, 0},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END};
#endif // #if defined(TARGET_NANOS)

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

void sample_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                if (G_io_apdu_buffer[0] != CLA) {
                    THROW(0x6E00);
                }

                switch (G_io_apdu_buffer[1]) {
                case 0x05: {
                    if (!write_metadata(G_io_apdu_buffer + 5,
                                        G_io_apdu_buffer[4])) {
                        THROW(EXCEPTION);
                    }
                    break;
                }

                case 0x06: {
                    uint32_t offset;
                    offset = find_next_metadata(currentMetadataOffset);
                    currentMetadataOffset = offset;
                    os_memmove(G_io_apdu_buffer,
                               &N_storage.metadatas[offset + 2],
                               N_storage.metadatas[offset]);
                    tx = N_storage.metadatas[offset];
                    break;
                }

                case 0x07: {
                    uint32_t offset;
                    offset = find_previous_metadata(currentMetadataOffset);
                    currentMetadataOffset = offset;
                    os_memmove(G_io_apdu_buffer,
                               &N_storage.metadatas[offset + 2],
                               N_storage.metadatas[offset]);
                    tx = N_storage.metadatas[offset];
                    break;
                }

                case 0x08: {
                    reset_metadatas();
                    break;
                }
                }
                // default no error
                THROW(0x9000);
            }
            CATCH_OTHER(e) {
                switch (e & 0xFFFFF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    // TODO here: error processing, memory wipes ?
                    break;
                case 0x9000:
                    // ok
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed
    button_push_callback_t callback;

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        // switch handle to bkb
        if (mode == MODE_CREATE) {
            callback = ux.button_push_handler;
            ux.button_push_handler = handle_button_push_create_entry;
        }
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        // rollback
        if (mode == MODE_CREATE) {
            ux.button_push_handler = callback;
        }
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({
            if (mode == MODE_CREATE) {
                bkb_display_ready();
            }
        });
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            // a pin lock is undergoing ?
            if (UX_ALLOWED) {
                if (mode != MODE_CREATE) {
                    if (ux_step_count) {
                        // prepare next screen
                        ux_step = (ux_step + 1) % ux_step_count;
                        // redisplay screen
                        UX_REDISPLAY();
                    }
                    break;
                } else {
                    bkb_animate(100);
                }
            }
        });
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
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

    UX_INIT();

    // ensure exception will work as planned
    os_boot();

    BEGIN_TRY {
        TRY {
            io_seproxyhal_init();

            if (N_storage.magic != STORAGE_MAGIC) {
                uint32_t magic;
                uint8_t dummy[2];
                magic = STORAGE_MAGIC;
                nvm_write(&N_storage.magic, (void *)&magic, sizeof(uint32_t));
                os_memset(dummy, 0, 2);
                nvm_write(N_storage.metadatas, (void *)&dummy, 2);
            }

            USB_power(1);

            mode = MODE_NONE;
            UX_MENU_DISPLAY(0, menu_main, NULL);

            currentMetadataOffset = 0;
            firstMetadataFound = 0;

            sample_main();
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
