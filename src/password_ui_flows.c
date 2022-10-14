#include <usbd_hid_impl.h>
#include <hid_mapping.h>
#include <string.h>
#include <stdbool.h>

#include "keyboard.h"
#include "password_ui_flows.h"
#include "globals.h"
#include "password_typing.h"
#include "metadata.h"
#include "dispatcher.h"
#include "sw.h"
#include "io.h"

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
keyboard_ctx_t G_keyboard_ctx;

const message_pair_t ERR_MESSAGES[] = {
    {"", 1, "", 1},                               // OK
    {"Write Error", 12, "Database is full", 17},  // ERR_NO_MORE_SPACE_AVAILABLE
    {"Write Error",
     12,
     "Database should be repaired, please contact Ledger Support",
     59},                                                // ERR_CORRUPTED_METADATA
    {"Erase Error", 12, "Database already empty", 23}};  // ERR_NO_METADATA

char line_buffer_1[16];
char line_buffer_2[21];

///////////////////////////////// USER APPROVAL //////////////////////////////////////////////

// clang-format off
UX_STEP_CB(
request_user_approval_step,
pnn,
app_state.user_approval = true; dispatch(),
{
    &C_icon_validate_14,
    line_buffer_1,
    line_buffer_2,
});
UX_STEP_CB(
generic_cancel_step,
pb,
send_sw(SW_CONDITIONS_OF_USE_NOT_SATISFIED); ui_idle(),
{
    &C_icon_back,
    "Cancel",
});
// clang-format on

UX_FLOW(request_user_approval_flow, &request_user_approval_step, &generic_cancel_step);

void ui_request_user_approval(message_pair_t* msg) {
    strncpy(line_buffer_1, (char*) PIC(msg->first), sizeof(line_buffer_1));
    strncpy(line_buffer_2, (char*) PIC(msg->second), sizeof(line_buffer_2));
    ux_flow_init(0, request_user_approval_flow, NULL);
}

//////////////////////////////////// TYPE PASSWORD ///////////////////////////////////////////

uint16_t current_entry_index;
int8_t previous_location;  // max left: -1, middle: 0, max right: 1
void (*selector_callback)();

void display_next_entry(bool is_upper_border);
void get_current_entry_name();
void select_password_and_apply_cb();
void type_password_cb(size_t offset);
void show_password_cb(size_t offset);
void reset_password_cb(size_t offset);

// clang-format off
UX_STEP_INIT(
select_password_upper_border_step,
NULL,
NULL,
{
    display_next_entry(true);
});
UX_STEP_CB_INIT(
select_password_current_entry_step,
bn,
get_current_entry_name(),
select_password_and_apply_cb(),
{
    line_buffer_1,
    line_buffer_2,
});
UX_STEP_INIT(
select_password_lower_border_step,
NULL,
NULL,
{
    display_next_entry(false);
});
// clang-format on

UX_FLOW(select_password_flow,
        &select_password_upper_border_step,
        &select_password_current_entry_step,
        &select_password_lower_border_step);

void display_type_password_flow() {
    current_entry_index = 0;
    previous_location = -1;
    selector_callback = type_password_cb;
    ux_flow_init(0, select_password_flow, NULL);
}

void display_show_password_flow() {
    current_entry_index = 0;
    previous_location = -1;
    selector_callback = show_password_cb;
    ux_flow_init(0, select_password_flow, NULL);
}

void display_reset_password_flow() {
    current_entry_index = 0;
    previous_location = -1;
    selector_callback = reset_password_cb;
    ux_flow_init(0, select_password_flow, NULL);
}

void display_next_entry(bool is_upper_border) {
    if (is_upper_border) {
        if (previous_location != -1) {
            if (current_entry_index > 0) {
                current_entry_index--;
            } else {
                current_entry_index = N_storage.metadata_count;  // Loop back
            }
        }
        ux_flow_next();
    }
    if (!is_upper_border) {
        if (current_entry_index < N_storage.metadata_count) {
            current_entry_index++;
        } else {
            current_entry_index = 0;
        }
        ux_flow_prev();
    }
}

void get_current_entry_name() {
    size_t offset = get_metadata(current_entry_index);
    if (offset == -1UL) {
        strlcpy(line_buffer_1, "", 1);
        strlcpy(line_buffer_2, "Cancel", 7);
        previous_location = 1;
    } else {
        SPRINTF(line_buffer_1, "Password %d/%d", current_entry_index + 1, N_storage.metadata_count);
        memcpy(line_buffer_2, (void*) METADATA_NICKNAME(offset), METADATA_NICKNAME_LEN(offset));
        line_buffer_2[METADATA_NICKNAME_LEN(offset)] = '\0';
        previous_location = 0;
    }
}

void select_password_and_apply_cb() {
    size_t offset = get_metadata(current_entry_index);
    // Check if user didn't click on "cancel"
    if (offset != -1UL) {
        selector_callback(offset);
    } else {
        ui_idle();
    }
}

void type_password_cb(size_t offset) {
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    type_password((uint8_t*) METADATA_NICKNAME(offset),
                  METADATA_NICKNAME_LEN(offset),
                  NULL,
                  enabledSets,
                  (const uint8_t*) PIC(DEFAULT_MIN_SET),
                  20);
    ui_idle();
}

void reset_password_cb(size_t offset) {
    error_type_t err = erase_metadata(offset);
    if (err != OK) {
        ui_error(ERR_MESSAGES[err]);
        return;
    }
    ui_idle();
}

//////////////////////////////// SHOW PASSWORD ///////////////////////////////////////////////

// clang-format off
UX_STEP_CB(
show_password_step,
bnnn_paging,
ui_idle(),
{
    line_buffer_1,
    line_buffer_2,
});

UX_FLOW(show_password_flow,
        &show_password_step);
// clang-format on

void show_password_cb(size_t offset) {
    memcpy(line_buffer_1, (void*) METADATA_NICKNAME(offset), METADATA_NICKNAME_LEN(offset));
    line_buffer_1[METADATA_NICKNAME_LEN(offset)] = '\0';
    unsigned char enabledSets = METADATA_SETS(offset);
    if (enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    type_password((uint8_t*) METADATA_NICKNAME(offset),
                  METADATA_NICKNAME_LEN(offset),
                  (uint8_t*) line_buffer_2,
                  enabledSets,
                  (const uint8_t*) PIC(DEFAULT_MIN_SET),
                  20);
    ux_flow_init(0, show_password_flow, NULL);
}

//////////////////////////////// CREATE NEW PASSWORD ///////////////////////////////////////////////

unsigned char G_create_classes;

void get_current_charset_setting_value(uint8_t symbols_bitflag);
void toggle_password_setting(uint8_t caller_id, uint8_t symbols_bitflag);
void create_password_entry();
void display_nickname_explanation();
void enter_password_nickname();

// clang-format off
UX_STEP_CB_INIT(
new_password_uppercase_step,
nn,
get_current_charset_setting_value(UPPERCASE_BITFLAG),
toggle_password_setting(0, UPPERCASE_BITFLAG),
{
    line_buffer_2,
    "uppercase",
});
UX_STEP_CB_INIT(
new_password_lowercase_step,
nn,
get_current_charset_setting_value(LOWERCASE_BITFLAG),
toggle_password_setting(1, LOWERCASE_BITFLAG),
{
    line_buffer_2,
    "lowercase",
});
UX_STEP_CB_INIT(
new_password_numbers_step,
nn,
get_current_charset_setting_value(NUMBERS_BITFLAG),
toggle_password_setting(2, NUMBERS_BITFLAG),
{
    line_buffer_2,
    "numbers",
});
UX_STEP_CB_INIT(
new_password_bars_step,
nn,
get_current_charset_setting_value(BARS_BITFLAG),
toggle_password_setting(3, BARS_BITFLAG),
{
    line_buffer_2,
    "-/ /_",
});
UX_STEP_CB_INIT(
new_password_ext_step,
nn,
get_current_charset_setting_value(EXT_SYMBOLS_BITFLAG),
toggle_password_setting(4, EXT_SYMBOLS_BITFLAG),
{
    line_buffer_2,
    "ext symbols",
});
UX_STEP_CB(
new_password_approve_step,
pb,
#if defined(TARGET_NANOS)
display_nickname_explanation(),
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
enter_password_nickname(),
#endif
{
    &C_icon_validate_14,
    "Create password",
});
// clang-format on

UX_FLOW(new_password_flow,
        &new_password_uppercase_step,
        &new_password_lowercase_step,
        &new_password_numbers_step,
        &new_password_bars_step,
        &new_password_ext_step,
        &new_password_approve_step,
        &generic_cancel_step);

void display_new_password_flow(const ux_flow_step_t* const start_step) {
    if (start_step == NULL) {
        G_create_classes = 0x07;  // default: lowercase, uppercase, numbers only
    }
    ux_flow_init(0, new_password_flow, start_step);
}

void get_current_charset_setting_value(uint8_t symbols_bitflag) {
    if (G_create_classes & symbols_bitflag) {
        strlcpy(line_buffer_2, "With", 5);
    } else {
        strlcpy(line_buffer_2, "Without", 8);
    }
}

void toggle_password_setting(uint8_t caller_id, uint8_t symbols_bitflag) {
    G_create_classes ^= symbols_bitflag;
    display_new_password_flow(new_password_flow[caller_id]);
}

void create_password_entry() {
    // use the G_io_seproxyhal_spi_buffer as temp buffer to build the entry (and include the
    // requested set of chars)
    memmove(G_io_seproxyhal_spi_buffer + 1,
            G_keyboard_ctx.words_buffer,
            strlen(G_keyboard_ctx.words_buffer));
    // use the requested classes from the user
    G_io_seproxyhal_spi_buffer[0] = G_create_classes;
    // add the metadata
    error_type_t err =
        write_metadata(G_io_seproxyhal_spi_buffer, 1 + strlen(G_keyboard_ctx.words_buffer));
    if (err != OK) {
        ui_error(ERR_MESSAGES[err]);
        return;
    }
    ui_idle();
}

void enter_password_nickname() {
#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    strlcpy(G_keyboard_ctx.title, "Enter nickname", 15);
#endif
    memset(G_keyboard_ctx.words_buffer, 0, sizeof(G_keyboard_ctx.words_buffer));
    screen_text_keyboard_init(G_keyboard_ctx.words_buffer, MAX_METANAME, create_password_entry);
}

// clang-format off
UX_STEP_CB(
explain_password_nickname_step,
nn,
enter_password_nickname(),
{
    "Enter a nickname for",
    "this new password",
});
// clang-format on

UX_FLOW(explain_password_nickname_flow, &explain_password_nickname_step);

void display_nickname_explanation() {
    ux_flow_init(0, explain_password_nickname_flow, NULL);
}

// clang-format off
UX_STEP_NOCB(
err_corrupted_memory_step,
bnnn_paging,
{
    line_buffer_1,
    line_buffer_2,
});
// clang-format on

UX_FLOW(err_corrupted_memory_flow, &err_corrupted_memory_step, &generic_cancel_step);

void ui_error(message_pair_t err) {
    strlcpy(line_buffer_1, (char*) PIC(err.first), err.first_len);
    strlcpy(line_buffer_2, (char*) PIC(err.second), err.second_len);
    ux_flow_init(0, err_corrupted_memory_flow, NULL);
}

/////////////////////////////////// SETTINGS ////////////////////////////////////////////

void display_change_keyboard_flow(const ux_flow_step_t* const start_step);
void display_reset_password_list_flow();
void get_current_pressEnterAfterTyping_setting_value();
void switch_setting_pressEnterAfterTyping();

// clang-format off
UX_STEP_CB(
settings_change_keyboard_step,
nn,
display_change_keyboard_flow(NULL),
{
    "Change keyboard",
    "layout",
});
UX_STEP_CB(
settings_reset_password_list_step,
nn,
display_reset_password_list_flow(),
{
    "Erase the list",
    "of passwords",
});
UX_STEP_CB_INIT(
settings_pressEnterAfterTyping_step,
nn,
get_current_pressEnterAfterTyping_setting_value(),
switch_setting_pressEnterAfterTyping(),
{
    line_buffer_2,
    "after typing",
});
// clang-format on

UX_FLOW(settings_flow,
        &settings_change_keyboard_step,
        &settings_reset_password_list_step,
        &settings_pressEnterAfterTyping_step,
        &generic_cancel_step,
        FLOW_LOOP);

void display_settings_flow(const ux_flow_step_t* const start_step) {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, settings_flow, start_step);
}

void get_current_pressEnterAfterTyping_setting_value() {
    if (N_storage.press_enter_after_typing) {
        strlcpy(line_buffer_2, "Press Enter", 12);
    } else {
        strlcpy(line_buffer_2, "Don't press Enter", 18);
    }
}

void switch_setting_pressEnterAfterTyping() {
    bool new_value = !N_storage.press_enter_after_typing;
    nvm_write((void*) &N_storage.press_enter_after_typing, (void*) &new_value, sizeof(new_value));
    display_settings_flow(&settings_pressEnterAfterTyping_step);
}

////////////////////////// SETTINGS - CHANGE KEYBOARD LAYOUT //////////////////////////////////////

bagl_icon_details_t is_selected_icon;

void get_current_keyboard_setting_value(hid_mapping_t mapping);
void enter_keyboard_setting(uint8_t caller_id, hid_mapping_t mapping);

// clang-format off
UX_STEP_CB_INIT(
qwerty_step,
pb,
get_current_keyboard_setting_value(HID_MAPPING_QWERTY),
enter_keyboard_setting(0, HID_MAPPING_QWERTY),
{
    &is_selected_icon,
    "Qwerty",
});
UX_STEP_CB_INIT(
qwerty_international_step,
pb,
get_current_keyboard_setting_value(HID_MAPPING_QWERTY_INTL),
enter_keyboard_setting(1, HID_MAPPING_QWERTY_INTL),
{
    &is_selected_icon,
    #if defined(TARGET_NANOS)
    "Qwerty Intl",
    #elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    "Qwerty International",
    #endif
});
UX_STEP_CB_INIT(
azerty_step,
pb,
get_current_keyboard_setting_value(HID_MAPPING_AZERTY),
enter_keyboard_setting(2, HID_MAPPING_AZERTY),
{
    &is_selected_icon,
    "Azerty",
});
// clang-format on

UX_FLOW(change_keyboard_flow,
        &qwerty_step,
        &qwerty_international_step,
        &azerty_step,
        &generic_cancel_step,
        FLOW_LOOP);

void display_change_keyboard_flow(const ux_flow_step_t* const start_step) {
    ux_flow_init(0, change_keyboard_flow, start_step);
}

void get_current_keyboard_setting_value(hid_mapping_t mapping) {
    if (N_storage.keyboard_layout == mapping) {
        is_selected_icon = C_icon_validate_14;
    } else {
        memset(&is_selected_icon, 0, sizeof(is_selected_icon));
    }
}

void enter_keyboard_setting(uint8_t caller_id, hid_mapping_t mapping) {
    if (N_storage.keyboard_layout != 0) {
        nvm_write((void*) &N_storage.keyboard_layout, (void*) &mapping, sizeof(hid_mapping_t));
        display_change_keyboard_flow(change_keyboard_flow[caller_id]);
    } else {
        // This case only happens at application first launch
        nvm_write((void*) &N_storage.keyboard_layout, (void*) &mapping, sizeof(hid_mapping_t));
        ui_idle();
    }
}

////////////////// SETTINGS - RESET ALL PASSWORDS //////////////////////////////////////

void reset_password_list();

// clang-format off
UX_STEP_CB(
reset_password_list_step,
pnn,
reset_password_list(),
{
    &C_icon_warning,
    "Reset the",
    "password list ?",
});
// clang-format on

UX_FLOW(reset_password_list_flow, &reset_password_list_step, &generic_cancel_step);

void display_reset_password_list_flow() {
    ux_flow_init(0, reset_password_list_flow, NULL);
}

void reset_password_list() {
    reset_metadatas();
    ui_idle();
}

///////////////////////////////// INIT / IDLE //////////////////////////////////////////////

// clang-format off
UX_STEP_CB(
idle_type_password_step,
pb,
display_type_password_flow(),
{
    &C_icon_bootloader,
    "Type password",
});
UX_STEP_CB(
idle_show_password_step,
pb,
display_show_password_flow(),
{
    &C_icon_eye,
    "Show password",
});
UX_STEP_CB(
idle_new_password_step,
pb,
display_new_password_flow(NULL),
{
    &C_icon_plus,
    "New password",
});
UX_STEP_CB(
idle_reset_password_step,
pb,
display_reset_password_flow(),
{
    &C_icon_crossmark,
    "Delete password",
});
UX_STEP_CB(
idle_settings_step,
pb,
display_settings_flow(NULL),
{
    &C_icon_coggle,
    "Settings",
});
UX_STEP_NOCB(
idle_version_step,
bn,
{
    "Version",
    APPVERSION,
});
UX_STEP_CB(
idle_quit_step,
pb,
os_sched_exit(-1),
{
    &C_icon_dashboard,
    "Quit",
});
// clang-format on

UX_FLOW(idle_flow,
        &idle_type_password_step,
        &idle_show_password_step,
        &idle_new_password_step,
        &idle_reset_password_step,
        &idle_settings_step,
        &idle_version_step,
        &idle_quit_step,
        FLOW_LOOP);

/* Used only when the application is first launched to setup the right keyboard*/
// clang-format off
UX_STEP_NOCB(
explanation_step,
nn,
{
    "Select the layout",
    "of your computer",
});
// clang-format on

UX_FLOW(setup_keyboard_at_init_flow,
        &explanation_step,
        &qwerty_step,
        &qwerty_international_step,
        &azerty_step);

void ui_idle() {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    if (N_storage.keyboard_layout != 0) {
        ux_flow_init(0, idle_flow, NULL);
    } else {
        ux_flow_init(0, setup_keyboard_at_init_flow, NULL);
    }
}
