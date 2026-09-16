#include <string.h>
#include "os.h"
#include "os_io_seproxyhal.h"
#include "usbd_ledger.h"

#include "hid_mapping.h"

#include "password_typing.h"
#include "globals.h"

#define REPORT_SIZE 8
static const uint8_t EMPTY_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SPACE_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_REPORT[REPORT_SIZE] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_LOCK_REPORT[REPORT_SIZE] =
    {0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t ENTER_REPORT[REPORT_SIZE] = {0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00};

#define DRBG_SEED_SIZE 32

/* Seed material for the password DRBG. It lives on type_password()'s stack and is wiped before
 * that function returns, rather than in a global with application lifetime. */
typedef struct entropy_ctx_s {
    bool provided;
    uint8_t seed[DRBG_SEED_SIZE];
} entropy_ctx_t;

static int entropy_provider(void *context, unsigned char *buffer, size_t bufferSize) {
    entropy_ctx_t *entropy_ctx = (entropy_ctx_t *) context;

    if ((entropy_ctx == NULL) || entropy_ctx->provided) {
        return 1;
    }
    /* The DRBG requests MBEDTLS_CTR_DRBG_ENTROPY_LEN bytes; refuse rather than overrun its
     * buffer if that ever drops below the seed we hold. */
    if (bufferSize < sizeof(entropy_ctx->seed)) {
        return 1;
    }
    memcpy(buffer, entropy_ctx->seed, sizeof(entropy_ctx->seed));
    entropy_ctx->provided = true;
    return 0;
}

#ifndef TESTING
static void usb_write_wait(unsigned char *buf) {
    USBD_LEDGER_send(USBD_LEDGER_CLASS_HID_KBD, 0, buf, REPORT_SIZE, 0);
    os_io_seph_cmd_general_status();
}
#else
static void usb_write_wait(__attribute__((unused)) unsigned char *buf) {
    return;
}
#endif  // TESTING

/* Type the generated password on the host keyboard. Kept separate from type_password() so the
 * seed handling there reads as one short linear sequence. */
static bool type_password_over_hid(const uint8_t *password, uint32_t size) {
    uint8_t report[REPORT_SIZE] = {0};
    const uint32_t led_status = G_led_status;
    uint32_t i;
    bool ok = true;

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
        if (!map_char(N_storage.keyboard_layout, password[i], report)) {
            // Stop typing, but still fall through to restore the host keyboard state below.
            ok = false;
            break;
        }

        usb_write_wait(report);
        if (report[0] & SHIFT_KEY) {
            usb_write_wait((uint8_t *) CAPS_REPORT);
        } else {
            usb_write_wait((uint8_t *) EMPTY_REPORT);
        }

        // for international keyboard, make sure to insert space after special symbols
        if (N_storage.keyboard_layout == HID_MAPPING_QWERTY_INTL) {
            switch (password[i]) {
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

    if (ok && N_storage.press_enter_after_typing) {
        // press enter
        usb_write_wait((uint8_t *) ENTER_REPORT);
        usb_write_wait((uint8_t *) EMPTY_REPORT);
    }

    explicit_bzero(report, sizeof(report));
    return ok;
}

bool type_password(uint8_t *data,
                   uint32_t dataSize,
                   uint8_t *out,
                   setmask_t setMask,
                   const uint8_t *minFromSet,
                   uint32_t size) {
    uint32_t derive[9];
    uint8_t tmp[64];
    uint32_t i;
    entropy_ctx_t entropy_ctx = {0};
    mbedtls_ctr_drbg_context ctx;

    if (cx_hash_sha256(data, dataSize, tmp, sizeof(tmp)) != CX_SHA256_SIZE) {
        explicit_bzero(tmp, sizeof(tmp));
        return false;
    }
    derive[0] = DERIVE_PASSWORD_PATH;
    for (i = 0; i < 8; i++) {
        /* The digest bytes are promoted to a signed int before shifting, so a byte with its
         * high bit set shifted by 24 is not representable and the result is undefined. Cast to
         * uint32_t first. The packed value is unchanged on the target toolchains, so derived
         * passwords stay the same -- this removes the reliance on undefined behaviour, which
         * an optimisation or compiler change could otherwise turn into different passwords. */
        derive[i + 1] = 0x80000000u | ((uint32_t) tmp[4 * i] << 24) |
                        ((uint32_t) tmp[4 * i + 1] << 16) | ((uint32_t) tmp[4 * i + 2] << 8) |
                        ((uint32_t) tmp[4 * i + 3]);
    }

    if (os_derive_bip32_no_throw(CX_CURVE_SECP256K1, derive, 9, tmp, tmp + 32) != CX_OK) {
        explicit_bzero(derive, sizeof(derive));
        explicit_bzero(tmp, sizeof(tmp));
        return false;
    }

    /* tmp holds the derived private key and chain code; the path in derive is no longer
     * needed either. Both go as soon as the seed has been hashed out of them. */
    const bool seed_hashed =
        cx_hash_sha256(tmp, 64, entropy_ctx.seed, sizeof(entropy_ctx.seed)) == CX_SHA256_SIZE;
    explicit_bzero(tmp, sizeof(tmp));
    explicit_bzero(derive, sizeof(derive));
    if (!seed_hashed) {
        // Without this the all-zero seed buffer would be used, yielding a wrong password.
        explicit_bzero(&entropy_ctx, sizeof(entropy_ctx));
        return false;
    }

    mbedtls_ctr_drbg_init(&ctx);
    const int seeded = mbedtls_ctr_drbg_seed(&ctx, entropy_provider, &entropy_ctx, NULL, 0);
    /* The DRBG has absorbed the seed, so drop our copy here rather than keeping it alive for
     * the whole generation: past this point nothing but the DRBG state is secret. */
    explicit_bzero(&entropy_ctx, sizeof(entropy_ctx));
    if (seeded != 0) {
        mbedtls_ctr_drbg_free(&ctx);
        return false;
    }

    bool generated;
    if (out != NULL) {
        generated = generate_password(&ctx, setMask, minFromSet, out, size);
        if (!generated) {
            /* Partially generated characters, and no terminator was written: leave the
             * caller's buffer empty rather than holding an unterminated fragment. */
            explicit_bzero(out, size + 1);
        }
        // On success the caller owns `out` and clears it once it is done displaying it.
    } else {
        generated = generate_password(&ctx, setMask, minFromSet, tmp, size);
        if (generated) {
            generated = type_password_over_hid(tmp, size);
        }
        // tmp held the plaintext password.
        explicit_bzero(tmp, sizeof(tmp));
    }

    mbedtls_ctr_drbg_free(&ctx);
    return generated;
}
