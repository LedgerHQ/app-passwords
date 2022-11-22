#include <os.h>
#include <string.h>
#include <stdint.h>
#include <ux.h>

#if !defined(TARGET_FATSTACKS)

#include "keyboard.h"

const char* const screen_keyboard_classes_elements[] = {
    // when first letter is already entered
    "abcdefghijklmnopqrstuvwxyz\b\n\r",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ\b\n\r",
    "0123456789 '\"`&/?!:;.,~*$=+-[](){}<>\\_#@|%\b\n\r",
    // when first letter is not entered yet
    "abcdefghijklmnopqrstuvwxyz\r",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ\r",
    "0123456789 '\"`&/?!:;.,~*$=+-[](){}<>\\_#@|%\r",
};

#define GET_CHAR(char_class, char_idx) \
    ((char*) PIC(screen_keyboard_classes_elements[char_class]))[char_idx]

// these icons will be centered
const bagl_icon_details_t* const screen_keyboard_classes_icons[] = {
    &C_icon_lowercase,
    &C_icon_uppercase,
    &C_icon_digits,
    &C_icon_backspace,
    &C_icon_validate,
    &C_icon_classes,
#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    &C_icon_lowercase_invert,
    &C_icon_uppercase_invert,
    &C_icon_digits_invert,
    &C_icon_backspace_invert,
    &C_icon_validate_invert,
    &C_icon_classes_invert,
#endif
};

const bagl_element_t* screen_keyboard_class_callback(unsigned int event, unsigned int value);
appmain_t screen_keyboard_validation;
char* screen_keyboard_buffer;
unsigned int screen_keyboard_buffer_maxsize;

#define PP_BUFFER screen_keyboard_buffer

void screen_keyboard_render_icon(unsigned int value) {
    const bagl_icon_details_t* icon;
#if defined(TARGET_NANOS)
    icon = (bagl_icon_details_t*) PIC(screen_keyboard_classes_icons[value]);
    G_ux.tmp_element.component.y = 5;
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    uint8_t inverted = G_ux.tmp_element.component.userid == 0x02;
    icon = (bagl_icon_details_t*) PIC(screen_keyboard_classes_icons[value + (inverted ? 6 : 0)]);
    G_ux.tmp_element.component.y -= 7;
#endif
    G_ux.tmp_element.component.x += G_ux.tmp_element.component.width / 2 - icon->width / 2;
    G_ux.tmp_element.component.width = icon->width;
    G_ux.tmp_element.component.height = icon->height;
    G_ux.tmp_element.component.type = BAGL_ICON;
    G_ux.tmp_element.component.icon_id = 0;
    G_ux.tmp_element.text = (const char*) icon;
}

const bagl_element_t* screen_keyboard_item_callback(unsigned int event, unsigned int value) {
    switch (event) {
        case KEYBOARD_ITEM_VALIDATED:
            // depending on the chosen class, interpret the click
            if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\b') {
                if (strlen(PP_BUFFER)) {
                    PP_BUFFER[strlen(PP_BUFFER) - 1] = 0;
                    goto redisplay_current_class;
                }
            } else if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\r') {
                // go back to classes display
                screen_common_keyboard_init(0,
                                            G_keyboard_ctx.onboarding_step,
                                            (strlen(PP_BUFFER) ? 5 : 3),
                                            screen_keyboard_class_callback);
                return NULL;
            } else if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\n') {
                screen_keyboard_validation();
                return NULL;
            } else {
                // too much character entered already
                if (strlen(PP_BUFFER) >=
                    screen_keyboard_buffer_maxsize /*sizeof(G_keyboard_ctx.words_buffer) - 10*/
                    /*max pin + pin length*/) {
                    // validate entry. for the user to continue entering if needed (chunking)
                    screen_keyboard_validation();
                    return NULL;
                }
                // append the char and display classes again
                PP_BUFFER[strlen(PP_BUFFER)] = GET_CHAR(G_keyboard_ctx.onboarding_step, value);
                PP_BUFFER[strlen(PP_BUFFER)] = 0;

            redisplay_current_class:
                // redisplay the correct class depending on the current number of entered digits
                G_keyboard_ctx.onboarding_step =
                    (G_keyboard_ctx.onboarding_step % 3) + (strlen(PP_BUFFER) ? 0 : 3);
                screen_common_keyboard_init(
                    0,
                    (event == KEYBOARD_ITEM_VALIDATED && strlen(PP_BUFFER) == 0)
                        ? 0
                        : COMMON_KEYBOARD_INDEX_UNCHANGED,
                    strlen((char*) PIC(
                        screen_keyboard_classes_elements[G_keyboard_ctx.onboarding_step])),
                    screen_keyboard_item_callback);
                return NULL;
            }

        case KEYBOARD_RENDER_ITEM:
            G_ux.tmp_element.text = G_ux.string_buffer;
            memset(G_ux.string_buffer, 0, 3);
            if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\b') {
                value = 3;
                goto set_bitmap;
            } else if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\r') {
                value = 5;
                goto set_bitmap;
            } else if (GET_CHAR(G_keyboard_ctx.onboarding_step, value) == '\n') {
                value = 4;

            set_bitmap:
                screen_keyboard_render_icon(value);
            } else {
                G_ux.string_buffer[0] = GET_CHAR(G_keyboard_ctx.onboarding_step, value);
            }
            break;

        case KEYBOARD_RENDER_WORD: {
            unsigned int l = strlen(PP_BUFFER);

            G_ux.string_buffer[0] = '_';
            G_ux.string_buffer[1] = 0;

            if (value < 8) {
                if (l <= 8) {
                    if (l > value) {
                        G_ux.string_buffer[0] = PP_BUFFER[value];
                    } else {
                        G_ux.string_buffer[0] = '_';
                    }
                } else {
                    // first char is '...' to notify continuing
                    if (value == 0) {
                        G_ux.string_buffer[0] = '.';
                        G_ux.string_buffer[1] = '.';
                        G_ux.string_buffer[2] = '.';
                        G_ux.string_buffer[3] = 0;
                    } else {
                        G_ux.string_buffer[0] = (PP_BUFFER + l - 8)[value];
                    }
                }
            }
            // ensure font is left aligned
            G_ux.tmp_element.text = G_ux.string_buffer;
            break;
        }
    }
    // update element display
    return &G_ux.tmp_element;
}

const bagl_element_t* screen_keyboard_class_callback(unsigned int event, unsigned int value) {
    switch (event) {
        case KEYBOARD_ITEM_VALIDATED:
            switch (value) {
                case 3:
                    // backspace
                    if (strlen(PP_BUFFER)) {
                        PP_BUFFER[strlen(PP_BUFFER) - 1] = 0;
                        screen_common_keyboard_init(
                            0,
                            strlen(PP_BUFFER) == 0 ? 0 : COMMON_KEYBOARD_INDEX_UNCHANGED,
                            (strlen(PP_BUFFER) ? 5 : 3),
                            screen_keyboard_class_callback);
                        return NULL;
                    }
                    break;
                case 4:
                    screen_keyboard_validation();
                    return NULL;

                case 0:
                case 1:
                case 2:
                    G_keyboard_ctx.onboarding_step = value + (strlen(PP_BUFFER) ? 0 : 3);
                    screen_common_keyboard_init(
                        0,
                        0,
                        strlen(&GET_CHAR(G_keyboard_ctx.onboarding_step, 0)),
                        screen_keyboard_item_callback);
                    return NULL;

                default:
                    // no validation
                    break;
            }
            break;
        case KEYBOARD_RENDER_ITEM:
            screen_keyboard_render_icon(value);
            break;
        case KEYBOARD_RENDER_WORD:
            // same as when drawing items
            return screen_keyboard_item_callback(event, value);
    }
    // update element display
    return &G_ux.tmp_element;
}

void screen_text_keyboard_init(char* buffer, unsigned int maxsize, appmain_t validation_callback) {
    screen_keyboard_buffer = buffer;
    screen_keyboard_buffer_maxsize = maxsize;
    screen_keyboard_validation = validation_callback;
    screen_common_keyboard_init(0, 0, 3, screen_keyboard_class_callback);
}

#endif
