#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nbgl_use_case.h"

// NBGL use case shown by the app
typedef enum {
    SCREEN_NONE = 0,
    SCREEN_STATUS,
    SCREEN_STATIC_REVIEW,
    SCREEN_CHOICE,
    SCREEN_GENERIC_CONFIGURATION,
    SCREEN_KEYBOARD,
    SCREEN_HOME_AND_SETTINGS,
    SCREEN_NAVIGABLE_CONTENT,
} screen_type_t;

// Arguments of the last NBGL use case, copied when the app passes them on its stack
typedef struct {
    screen_type_t type;
    // Title, or message of a status or a choice
    const char *title;
    const char *sub_message;
    const nbgl_icon_details_t *icon;
    bool success;
    uint8_t init_page;
    // Quit, back or status callback
    nbgl_callback_t quit;
    nbgl_choiceCallback_t choice;
    // Contents of a generic configuration, or the settings of the home page
    const nbgl_genericContents_t *contents;
    const nbgl_homeAction_t *home_action;
    nbgl_pageInfoLongPress_t long_press;
    nbgl_keyboardParams_t keyboard;
    nbgl_navCallback_t nav;
    nbgl_layoutTouchCallback_t touch;
} screen_t;

extern screen_t screen;
// Number of use cases shown since ui_test_setup()
extern size_t screen_count;
// Choices per page returned by nbgl_useCaseGetNbChoicesInPage()
extern uint8_t choices_per_page;

// Result of the password generation, and the text written for a displayed password
extern bool generation_result;
extern size_t generation_calls;
extern const uint8_t *generation_out;
#define GENERATED_PASSWORD "Generated-Password"

extern size_t app_exit_calls;
extern size_t dispatch_calls;
extern size_t sw_count;
extern uint16_t last_sw;

// Empty storage with a QWERTY layout and default settings, and every mock ready
void ui_test_setup(void);
void ui_test_teardown(void);

// Store a new password named `nickname` with the current settings
void add_password(const char *nickname);
