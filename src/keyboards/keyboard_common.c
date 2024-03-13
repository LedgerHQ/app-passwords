#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "string.h"

#include "keyboard.h"

#if !defined(SCREEN_SIZE_WALLET)

void bolos_ux_hslider3_init(unsigned int total_count) {
    G_keyboard_ctx.hslider3_total = total_count;
    switch (total_count) {
        case 0:
            G_keyboard_ctx.hslider3_before = BOLOS_UX_HSLIDER3_NONE;
            G_keyboard_ctx.hslider3_current = BOLOS_UX_HSLIDER3_NONE;
            G_keyboard_ctx.hslider3_after = BOLOS_UX_HSLIDER3_NONE;
            break;
        case 1:
            G_keyboard_ctx.hslider3_before = BOLOS_UX_HSLIDER3_NONE;
            G_keyboard_ctx.hslider3_current = 0;
            G_keyboard_ctx.hslider3_after = BOLOS_UX_HSLIDER3_NONE;
            break;
        case 2:
            G_keyboard_ctx.hslider3_before = BOLOS_UX_HSLIDER3_NONE;
            // G_keyboard_ctx.hslider3_before = 1; // full rotate
            G_keyboard_ctx.hslider3_current = 0;
            G_keyboard_ctx.hslider3_after = 1;
            break;
        default:
            G_keyboard_ctx.hslider3_before = total_count - 1;
            G_keyboard_ctx.hslider3_current = 0;
            G_keyboard_ctx.hslider3_after = 1;
            break;
    }
}

void bolos_ux_hslider3_set_current(unsigned int current) {
    // index is reachable ?
    if (G_keyboard_ctx.hslider3_total > current) {
        // reach it
        while (G_keyboard_ctx.hslider3_current != current) {
            bolos_ux_hslider3_next();
        }
    }
}

void bolos_ux_hslider3_next(void) {
    switch (G_keyboard_ctx.hslider3_total) {
        case 0:
        case 1:
            break;
        case 2:
            switch (G_keyboard_ctx.hslider3_current) {
                case 0:
                    G_keyboard_ctx.hslider3_before = 0;
                    G_keyboard_ctx.hslider3_current = 1;
                    G_keyboard_ctx.hslider3_after = BOLOS_UX_HSLIDER3_NONE;
                    break;
                case 1:
                    G_keyboard_ctx.hslider3_before = BOLOS_UX_HSLIDER3_NONE;
                    G_keyboard_ctx.hslider3_current = 0;
                    G_keyboard_ctx.hslider3_after = 1;
                    break;
            }
            break;
        default:
            G_keyboard_ctx.hslider3_before = G_keyboard_ctx.hslider3_current;
            G_keyboard_ctx.hslider3_current = G_keyboard_ctx.hslider3_after;
            G_keyboard_ctx.hslider3_after =
                (G_keyboard_ctx.hslider3_after + 1) % G_keyboard_ctx.hslider3_total;
            break;
    }
}

void bolos_ux_hslider3_previous(void) {
    switch (G_keyboard_ctx.hslider3_total) {
        case 0:
        case 1:
            break;
        case 2:
            switch (G_keyboard_ctx.hslider3_current) {
                case 0:
                    G_keyboard_ctx.hslider3_before = 0;
                    G_keyboard_ctx.hslider3_current = 1;
                    G_keyboard_ctx.hslider3_after = BOLOS_UX_HSLIDER3_NONE;
                    break;
                case 1:
                    G_keyboard_ctx.hslider3_before = BOLOS_UX_HSLIDER3_NONE;
                    G_keyboard_ctx.hslider3_current = 0;
                    G_keyboard_ctx.hslider3_after = 1;
                    break;
            }
            break;
        default:
            G_keyboard_ctx.hslider3_after = G_keyboard_ctx.hslider3_current;
            G_keyboard_ctx.hslider3_current = G_keyboard_ctx.hslider3_before;
            G_keyboard_ctx.hslider3_before =
                (G_keyboard_ctx.hslider3_before + G_keyboard_ctx.hslider3_total - 1) %
                G_keyboard_ctx.hslider3_total;
            break;
    }
}

#endif
