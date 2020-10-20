#include "password_typing.h"

#include "os.h"
#include "os_io_seproxyhal.h"
#include "usbd_hid_impl.h"

#include "shared_context.h"
#include "hid_mapping.h"


static const uint8_t EMPTY_REPORT[] =       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SPACE_REPORT[] =       {0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_REPORT[] =        {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t CAPS_LOCK_REPORT[] =   {0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t ENTER_REPORT[] =       {0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t entropyProvided;
uint8_t entropy[32];


int entropyProvider2(void *context, unsigned char *buffer, size_t bufferSize) {
    if (entropyProvided) {
        // PRINTF("no more entropy\n");
        return 1;
    }
    os_memcpy(buffer, entropy, 32);
    // PRINTF("entropy: %.*H\n", 32, entropy);
    entropyProvided = 1;
    return 0;
}

void io_usb_send_ep_wait(unsigned int ep, unsigned char* buf, unsigned int len, unsigned int timeout_cs) {
    io_usb_send_ep(ep, buf, len, 20);

    // wait until transfer timeout, or ended
    while (G_io_app.usb_ep_timeouts[ep&0x7F].timeout) {
        if (!io_seproxyhal_spi_is_status_sent()) {
            io_seproxyhal_general_status();
        }
        io_seproxyhal_spi_recv(G_io_seproxyhal_spi_buffer, sizeof(G_io_seproxyhal_spi_buffer), 0);
        io_seproxyhal_handle_event();
    }
}

void type_password(uint8_t *data, uint32_t dataSize, uint8_t *out,
                setmask_t setMask, const uint8_t *minFromSet,
                uint32_t size) {
    uint32_t derive[9];
    uint32_t led_status;
    uint8_t tmp[64];
    uint8_t i;
    uint8_t report[8];


    cx_hash_sha256(data, dataSize, tmp, sizeof(tmp));
    derive[0] = DERIVE_PASSWORD_PATH;
    for (i = 0; i < 8; i++) {
        derive[i + 1] = 0x80000000 |
                        (tmp[4 * i] << 24) | (tmp[4 * i + 1] << 16) |
                        (tmp[4 * i + 2] << 8) | (tmp[4 * i + 3]);
    }
    os_perso_derive_node_bip32(CX_CURVE_SECP256K1, derive, 9, tmp, tmp + 32);
    //PRINTF("pwseed %.*H\n", 64, tmp);
    cx_hash_sha256(tmp, 64, entropy, sizeof(entropy));
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

    //PRINTF("out: %.*H\n", size, out);
#if 0
    out = "A-B_C D\\E\"F#G$H%I&J'K*L+M,N.O/P:Q;R=S?T@U^V`W|X~Y[Z]a{b}c(d)e<f>g!hijklmnopqrstuvwxyz0123456789~e^e'e`e\"e\"\"e";
    size = strlen(out);
#endif

    os_memset(report, 0, sizeof(report));
    // Insert EMPTY_REPORT CAPS_REPORT EMPTY_REPORT to avoid undesired capital letter on KONSOLE
    led_status = G_led_status;
    io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);

    io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)CAPS_REPORT, 8, 20);
    io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);

    // toggle shift if set.
    if (led_status&2){
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)CAPS_LOCK_REPORT, 8, 20);
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);
    }
    for (i = 0; i < size; i++) {
        // If keyboard layout not initialized, use the default
        map_char(N_storage.keyboard_layout, out[i], report);
        io_usb_send_ep_wait(HID_EPIN_ADDR, report, 8, 20);
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);

        // for international keyboard, make sure to insert space after special symbols
        if (N_storage.keyboard_layout == HID_MAPPING_QWERTY_INTL) {
            switch(out[i]) {
                case '\"':
                case '\'':
                case '`':
                case '~':
                case '^':
                    // insert a extra space to validate the symbol
                    io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)SPACE_REPORT, 8, 20);
                    io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);
                    break;
            }
        }
    }
    // restore shift state
    if (led_status&2){
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)CAPS_LOCK_REPORT, 8, 20);
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);
    }

    if(N_storage.press_enter_after_typing){
        // press enter
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)ENTER_REPORT, 8, 20);
        io_usb_send_ep_wait(HID_EPIN_ADDR, (uint8_t*)EMPTY_REPORT, 8, 20);
    }
}