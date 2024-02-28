#include <string.h>
#include <os.h>
#include <os_io_seproxyhal.h>

#include <hid_mapping.h>
#include <usbd_hid_impl.h>

#include "password_typing.h"
#include "globals.h"

#define REPORT_SIZE 8
static const uint8_t EMPTY_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SPACE_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_REPORT[REPORT_SIZE] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_LOCK_REPORT[REPORT_SIZE] =
    {0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t ENTER_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00};

bool entropyProvided;
uint8_t entropy[32];

static int entropyProvider2(__attribute__((unused)) void *context,
                            unsigned char *buffer,
                            __attribute__((unused)) size_t bufferSize) {
    if (entropyProvided) {
        return 1;
    }
    memcpy(buffer, entropy, 32);
    entropyProvided = true;
    return 0;
}

#ifndef TESTING
static void usb_write_wait(unsigned char *buf) {
    io_usb_send_ep(HID_EPIN_ADDR, buf, REPORT_SIZE, 60);

    // wait until transfer timeout, or ended
    while (G_io_app.usb_ep_timeouts[HID_EPIN_ADDR & 0x7F].timeout) {
        if (!io_seproxyhal_spi_is_status_sent()) {
            io_seproxyhal_general_status();
        }
        io_seproxyhal_spi_recv(G_io_seproxyhal_spi_buffer, sizeof(G_io_seproxyhal_spi_buffer), 0);
        io_seproxyhal_handle_event();
    }
}
#else
static void usb_write_wait(__attribute__((unused)) unsigned char *buf) {
    return;
}
#endif  // TESTING

bool type_password(uint8_t *data,
                   uint32_t dataSize,
                   uint8_t *out,
                   setmask_t setMask,
                   const uint8_t *minFromSet,
                   uint32_t size) {
    uint32_t derive[9];
    uint32_t led_status;
    uint8_t tmp[64];
    uint8_t i;
    uint8_t report[REPORT_SIZE] = {0};

    cx_hash_sha256(data, dataSize, tmp, sizeof(tmp));
    derive[0] = DERIVE_PASSWORD_PATH;
    for (i = 0; i < 8; i++) {
        derive[i + 1] = 0x80000000 | (tmp[4 * i] << 24) | (tmp[4 * i + 1] << 16) |
                        (tmp[4 * i + 2] << 8) | (tmp[4 * i + 3]);
    }

    if (os_derive_bip32_no_throw(CX_CURVE_SECP256K1, derive, 9, tmp, tmp + 32) != CX_OK) {
        return false;
    }
    cx_hash_sha256(tmp, 64, entropy, sizeof(entropy));
    memset(tmp, 0, sizeof(tmp));
    entropyProvided = false;
    mbedtls_ctr_drbg_context ctx;
    mbedtls_ctr_drbg_init(&ctx);
    if (mbedtls_ctr_drbg_seed(&ctx, entropyProvider2, NULL, NULL, 0) != 0) {
        THROW(EXCEPTION);
    }
    if (out != NULL) {
        generate_password(&ctx, setMask, minFromSet, out, size);
        return true;
    }

    generate_password(&ctx, setMask, minFromSet, tmp, size);

    led_status = G_led_status;

    // Insert EMPTY_REPORT CAPS_REPORT EMPTY_REPORT to avoid undesired capital letter on KONSOLE
    usb_write_wait((uint8_t *) EMPTY_REPORT);
    usb_write_wait((uint8_t *) CAPS_REPORT);
    usb_write_wait((uint8_t *) EMPTY_REPORT);

    // toggle shift if set.
    if (led_status & 2) {
        usb_write_wait((uint8_t *) CAPS_LOCK_REPORT);
        usb_write_wait((uint8_t *) EMPTY_REPORT);
    }

    for (i = 0; i < size; i++) {
        // If keyboard layout not initialized, use the default
        map_char(N_storage.keyboard_layout, tmp[i], report);

        usb_write_wait(report);
        if (report[0] & SHIFT_KEY) {
            usb_write_wait((uint8_t *) CAPS_REPORT);
        } else {
            usb_write_wait((uint8_t *) EMPTY_REPORT);
        }

        // for international keyboard, make sure to insert space after special symbols
        if (N_storage.keyboard_layout == HID_MAPPING_QWERTY_INTL) {
            switch (tmp[i]) {
                case '\"':
                case '\'':
                case '`':
                case '~':
                case '^':
                    // insert a extra space to validate the symbol
                    usb_write_wait((uint8_t *) SPACE_REPORT);
                    usb_write_wait((uint8_t *) EMPTY_REPORT);
                    break;
            }
        }
    }
    usb_write_wait((uint8_t *) EMPTY_REPORT);
    // restore shift state
    if (led_status & 2) {
        usb_write_wait((uint8_t *) CAPS_LOCK_REPORT);
        usb_write_wait((uint8_t *) EMPTY_REPORT);
    }

    if (N_storage.press_enter_after_typing) {
        // press enter
        usb_write_wait((uint8_t *) ENTER_REPORT);
        usb_write_wait((uint8_t *) EMPTY_REPORT);
    }

    return true;
}
