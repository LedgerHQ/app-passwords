
#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "string.h"
#include "glyphs.h"

#include "keyboard.h"

#if defined(TARGET_NANOS)

const bagl_element_t screen_common_keyboard_elements[] = {

    // erase
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL},

    // typed word
    {{BAGL_RECTANGLE, 0x00, 18, 18, 110 - 18, 14, 0, 4, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0}, NULL},
    {{BAGL_LABELINE,
      0x10,
      128 / 2 - 12 / 2 - 40,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x11,
      128 / 2 - 12 / 2 - 30,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x12,
      128 / 2 - 12 / 2 - 20,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x13,
      128 / 2 - 12 / 2 - 10,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x14,
      128 / 2 - 12 / 2,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x15,
      128 / 2 - 12 / 2 + 10,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x16,
      128 / 2 - 12 / 2 + 20,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x17,
      128 / 2 - 12 / 2 + 30,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    {{BAGL_LABELINE,
      0x18,
      128 / 2 - 12 / 2 + 40,
      28,
      12,
      12,
      0,
      0,
      0,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},

    // slider elements
    {{BAGL_LINE, 0x06, 46, 8, 3, 1, 0, 0, 0, 0xFFFFFF, 0x000000, 0, 0}, NULL},
    {{BAGL_LINE, 0x07, 79, 8, 3, 1, 0, 0, 0, 0xFFFFFF, 0x000000, 0, 0}, NULL},

    // previous element
    {{BAGL_LABELINE,
      0x01,
      26,
      12,
      14,
      13,
      0,
      0,
      BAGL_FILL,
      0xFFFFFF,
      0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},

    // current item
    {{BAGL_RECTANGLE, 0x22, 57, 12 - 10, 14, 14, 0, 4, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0}, NULL},
    {{BAGL_LABELINE,
      0x02,
      58,
      12,
      12,
      13,
      0,
      0,
      BAGL_FILL,
      0x000000,
      0xFFFFFF,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},
    //{{BAGL_LABELINE                       , 0x02,  57,  12,  14,  13, 0, 4, BAGL_FILL, 0x000000,
    // 0xFFFFFF,
    // BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER|BAGL_FONT_ALIGNMENT_MIDDLE, 0
    // },
    // NULL },

    // next element
    {{BAGL_LABELINE,
      0x03,
      88,
      12,
      14,
      13,
      0,
      0,
      BAGL_FILL,
      0xFFFFFF,
      0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER,
      0},
     NULL},

    // left/rights icons
    {{BAGL_ICON, 0x00, 3, 12, 4, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, 0}, (const char*) &C_icon_left},
    {{BAGL_ICON, 0x00, 121, 12, 4, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, 0},
     (const char*) &C_icon_right},
};

const bagl_element_t* screen_common_keyboard_before_element_display_callback(
    const bagl_element_t* element) {
    const bagl_element_t* e;
    // copy element to be displayed
    memmove(&G_ux.tmp_element, (void*) PIC(element), sizeof(G_ux.tmp_element));

    switch (element->component.userid) {
        case 0x01:
            if (G_keyboard_ctx.hslider3_before == BOLOS_UX_HSLIDER3_NONE) {
                return 0;
            }
            return G_keyboard_ctx.keyboard_callback(KEYBOARD_RENDER_ITEM,
                                                    G_keyboard_ctx.hslider3_before);

        // current item (both line and invert rectangle)
        case 0x22:
        case 0x02:
            e = G_keyboard_ctx.keyboard_callback(KEYBOARD_RENDER_ITEM,
                                                 G_keyboard_ctx.hslider3_current);

            // if the current component to display is not TEXT, then don't display the invert
            // rectangle, to avoid graphic glitch
            if (element->component.userid == 0x22) {
                if (e->component.type == BAGL_ICON) {
                    return NULL;
                }
            }
            return e;
            break;

        case 0x03:
            if (G_keyboard_ctx.hslider3_after == BOLOS_UX_HSLIDER3_NONE) {
                return 0;
            }
            return G_keyboard_ctx.keyboard_callback(KEYBOARD_RENDER_ITEM,
                                                    G_keyboard_ctx.hslider3_after);

        case 0x06:
            if (G_keyboard_ctx.hslider3_before == BOLOS_UX_HSLIDER3_NONE) {
                return 0;  // don't display
            }
            break;

        case 0x07:
            if (G_keyboard_ctx.hslider3_after == BOLOS_UX_HSLIDER3_NONE) {
                return 0;  // don't display
            }
            break;

        default:
            if (element->component.userid & 0x10) {
                // request the xieth word char
                return G_keyboard_ctx.keyboard_callback(KEYBOARD_RENDER_WORD,
                                                        element->component.userid & 0x0F);
            }
            break;
    }
    // return the probably modded element by the callback function
    return &G_ux.tmp_element;
}

unsigned int screen_common_keyboard_button(unsigned int button_mask,
                                           unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);

    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:  // validate current digit

            // validate the item, and if accepted, then redisplay current screen, else don't draw
            // anything
            if (G_keyboard_ctx.keyboard_callback(KEYBOARD_ITEM_VALIDATED,
                                                 G_keyboard_ctx.hslider3_current)) {
                goto redraw;
            }
            break;

        case BUTTON_EVT_FAST | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            bolos_ux_hslider3_previous();
            goto redraw;

        case BUTTON_EVT_FAST | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            bolos_ux_hslider3_next();

        redraw:
            ux_stack_display(G_ux.stack_count - 1);
            break;
    }
    return 1;
}

void screen_common_keyboard_init(unsigned int stack_slot,
                                 unsigned int current_element,
                                 unsigned int nb_elements,
                                 keyboard_callback_t callback) {
    unsigned int current = G_keyboard_ctx.hslider3_current;
    ux_stack_init(stack_slot);

    // initiate the rotating modulo iterator
    bolos_ux_hslider3_init(nb_elements);
    if (current_element == COMMON_KEYBOARD_INDEX_UNCHANGED) {
        bolos_ux_hslider3_set_current(current);
    } else {
        bolos_ux_hslider3_set_current(current_element);
    }

    G_ux.stack[stack_slot].element_arrays[0].element_array = screen_common_keyboard_elements;
    G_ux.stack[stack_slot].element_arrays[0].element_array_count =
        ARRAYLEN(screen_common_keyboard_elements);
    G_ux.stack[stack_slot].element_arrays_count = 1;
    G_ux.stack[stack_slot].screen_before_element_display_callback =
        screen_common_keyboard_before_element_display_callback;  // used for each screen of the
                                                                 // validate pin flow
    G_ux.stack[stack_slot].button_push_callback = screen_common_keyboard_button;
    G_keyboard_ctx.keyboard_callback = callback;

    ux_stack_display(stack_slot);
}

#endif
