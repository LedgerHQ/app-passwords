#include "password_ui_flows.h"

#include "usbd_hid_impl.h"
#include "hid_mapping.h"
#include "string.h"
#include "stdbool.h"
#include "keyboard.h"

#include "shared_context.h"
#include "password_typing.h"
#include "metadata.h"

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
keyboard_ctx_t G_keyboard_ctx;

//////////////////////////////////// TYPE PASSWORD ///////////////////////////////////////////

char current_screen_type[10];
char current_entry_name[20];
uint8_t current_entry_index;
int8_t previous_location; // max left: -1, middle: 0, max right: 1
void (*selector_callback) ();

void display_next_entry(bool is_upper_border);
void get_current_entry_name();
void select_password_and_apply_cb();
void type_password_cb(size_t offset);
void reset_password_cb(size_t offset);

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
    current_screen_type,
    current_entry_name,
});
UX_STEP_INIT(
select_password_lower_border_step,
NULL,
NULL,
{
    display_next_entry(false);
});

UX_FLOW(select_password_flow,
&select_password_upper_border_step,
&select_password_current_entry_step,
&select_password_lower_border_step
);

void display_type_password_flow() {
    current_entry_index = 0;
    previous_location = -1;
    selector_callback = type_password_cb;
    ux_flow_init(0, select_password_flow, NULL);
}

void display_reset_password_flow() {
    current_entry_index = 0;
    previous_location = -1;
    selector_callback = reset_password_cb;
    ux_flow_init(0, select_password_flow, NULL);
}

void display_next_entry(bool is_upper_border){
    if(is_upper_border){
        if(previous_location != -1){
            if(current_entry_index > 0){
                current_entry_index--;
            }
            else{
                current_entry_index = N_storage.metadata_count; // Loop back
            }
        }
        ux_flow_next();
    }
    if(!is_upper_border){
        if(current_entry_index < N_storage.metadata_count){
            current_entry_index++;
        }
        else{
            current_entry_index = 0;
        }
        ux_flow_prev();
    }
}

void get_current_entry_name(){
    size_t offset = get_metadata(current_entry_index);
    if(offset == -1UL){
        strcpy(current_screen_type, "");
        strcpy(current_entry_name, "Cancel");
        previous_location = 1;
    }
    else{
        strcpy(current_screen_type, "Name");
        memcpy(current_entry_name, (void*)METADATA_PW(offset), METADATA_PWLEN(offset));
        current_entry_name[METADATA_PWLEN(offset) >= MAX_METANAME ? MAX_METANAME - 1 : METADATA_PWLEN(offset)] = '\0';
        previous_location = 0;
    }
}

void select_password_and_apply_cb(){
    size_t offset = get_metadata(current_entry_index);
    // Check if user didn't click on "cancel"
    if(offset != -1UL){
        selector_callback(offset);
    }
    ui_idle();
}

void type_password_cb(size_t offset){
    unsigned char enabledSets = METADATA_SETS(offset);
    if(enabledSets == 0) {
        enabledSets = ALL_SETS;
    }
    type_password(
        (uint8_t*)METADATA_PW(offset),METADATA_PWLEN(offset),
        NULL,
        enabledSets,
        (const uint8_t*)PIC(DEFAULT_MIN_SET),
        20);
}

void reset_password_cb(size_t offset){
    erase_metadata(offset);
}

//////////////////////////////// CREATE NEW PASSWORD ///////////////////////////////////////////////

unsigned char G_create_classes;
char setting_str_buffer[18];

void get_current_charset_setting_value(uint8_t symbols_bitflag);
void toggle_password_setting(uint8_t caller_id, uint8_t symbols_bitflag);
void create_password_entry();
void display_nickname_explanation();
void enter_password_nickname();

UX_STEP_CB_INIT(
new_password_uppercase_step,
nn,
get_current_charset_setting_value(UPPERCASE_BITFLAG),
toggle_password_setting(0, UPPERCASE_BITFLAG),
{
    setting_str_buffer,
    "uppercase",
});
UX_STEP_CB_INIT(
new_password_lowercase_step,
nn,
get_current_charset_setting_value(LOWERCASE_BITFLAG),
toggle_password_setting(1, LOWERCASE_BITFLAG),
{
    setting_str_buffer,
    "lowercase",
});
UX_STEP_CB_INIT(
new_password_numbers_step,
nn,
get_current_charset_setting_value(NUMBERS_BITFLAG),
toggle_password_setting(2, NUMBERS_BITFLAG),
{
    setting_str_buffer,
    "numbers",
});
UX_STEP_CB_INIT(
new_password_bars_step,
nn,
get_current_charset_setting_value(BARS_BITFLAG),
toggle_password_setting(3, BARS_BITFLAG),
{
    setting_str_buffer,
    "-/ /_",
});
UX_STEP_CB_INIT(
new_password_ext_step,
nn,
get_current_charset_setting_value(EXT_SYMBOLS_BITFLAG),
toggle_password_setting(4, EXT_SYMBOLS_BITFLAG),
{
    setting_str_buffer,
    "ext symbols",
});
UX_STEP_CB(
new_password_approve_step,
pb,
#if defined(TARGET_NANOS)
display_nickname_explanation(),
#elif defined(TARGET_NANOX)
enter_password_nickname(),
#endif
{
    &C_icon_validate_14,
    "Create password",
});
UX_STEP_CB(
new_password_cancel_step,
pb,
ui_idle(),
{
    &C_icon_back,
    "Cancel",
});

UX_FLOW(new_password_flow,
&new_password_uppercase_step,
&new_password_lowercase_step,
&new_password_numbers_step,
&new_password_bars_step,
&new_password_ext_step,
&new_password_approve_step,
&new_password_cancel_step
);

void display_new_password_flow(const ux_flow_step_t* const start_step) {
    if(start_step == NULL){
        G_create_classes = 0x07; // default: lowercase, uppercase, numbers only
    }
    ux_flow_init(0, new_password_flow, start_step);
}

void get_current_charset_setting_value(uint8_t symbols_bitflag){
    if(G_create_classes & symbols_bitflag){
        strcpy(setting_str_buffer, "With");
    }
    else{
        strcpy(setting_str_buffer, "Without");
    }
}

void toggle_password_setting(uint8_t caller_id, uint8_t symbols_bitflag){
    G_create_classes ^= symbols_bitflag;
    display_new_password_flow(new_password_flow[caller_id]);
}

void create_password_entry(){
    // use the G_io_seproxyhal_spi_buffer as temp buffer to build the entry (and include the requested set of chars)
    os_memmove(G_io_seproxyhal_spi_buffer+1, G_keyboard_ctx.words_buffer, strlen(G_keyboard_ctx.words_buffer));
    // use the requested classes from the user
    G_io_seproxyhal_spi_buffer[0] = G_create_classes;
    // add the metadata
    write_metadata(G_io_seproxyhal_spi_buffer, 1 + strlen(G_keyboard_ctx.words_buffer));

    ui_idle();
}

void enter_password_nickname(){
    #if defined(TARGET_NANOX)
        strcpy(G_keyboard_ctx.title, "Enter nickname");
    #endif
    os_memset(G_keyboard_ctx.words_buffer, 0, sizeof(G_keyboard_ctx.words_buffer));
    screen_text_keyboard_init(G_keyboard_ctx.words_buffer, 20, create_password_entry);
}

UX_STEP_CB(
explain_password_nickname_step,
nn,
enter_password_nickname(),
{
    "Enter a nickname for",
    "this new password",
});

UX_FLOW(explain_password_nickname_flow,
&explain_password_nickname_step
);

void display_nickname_explanation(){
    ux_flow_init(0, explain_password_nickname_flow, NULL);
}


/////////////////////////////////// SETTINGS ////////////////////////////////////////////

void display_change_keyboard_flow(const ux_flow_step_t* const start_step);
void display_reset_password_list_flow();
void get_current_pressEnterAfterTyping_setting_value();
void switch_setting_pressEnterAfterTyping();

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
    setting_str_buffer,
    "after typing",
});
UX_STEP_CB(
settings_cancel_step,
pb,
ui_idle(),
{
    &C_icon_back,
    "Cancel",
});

UX_FLOW(settings_flow,
&settings_change_keyboard_step,
&settings_reset_password_list_step,
&settings_pressEnterAfterTyping_step,
&settings_cancel_step,
FLOW_LOOP
);

void display_settings_flow(const ux_flow_step_t* const start_step) {
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, settings_flow, start_step);
}

void get_current_pressEnterAfterTyping_setting_value(){
    if(N_storage.press_enter_after_typing){
        strcpy(setting_str_buffer, "Press Enter");
    }
    else{
        strcpy(setting_str_buffer, "Don't press Enter");
    }
}

void switch_setting_pressEnterAfterTyping(){
    bool new_value = !N_storage.press_enter_after_typing;
    nvm_write((void*)&N_storage.press_enter_after_typing, (void*)&new_value, sizeof(new_value));
    display_settings_flow(&settings_pressEnterAfterTyping_step);
}

//////////////////////////////// SETTINGS - CHANGE KEYBOARD LAYOUT ///////////////////////////////////////////////

bagl_icon_details_t is_selected_icon;

void get_current_keyboard_setting_value(hid_mapping_t mapping);
void enter_keyboard_setting(uint8_t caller_id, hid_mapping_t mapping);

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
    #elif defined(TARGET_NANOX)
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
UX_STEP_CB(
change_keyboard_cancel_step,
pb,
ui_idle(),
{
    &C_icon_back,
    "Back",
});

UX_FLOW(change_keyboard_flow,
&qwerty_step,
&qwerty_international_step,
&azerty_step,
&change_keyboard_cancel_step,
FLOW_LOOP
);

void display_change_keyboard_flow(const ux_flow_step_t* const start_step) {
    ux_flow_init(0, change_keyboard_flow, start_step);
}

void get_current_keyboard_setting_value(hid_mapping_t mapping){
    if(N_storage.keyboard_layout == mapping){
        is_selected_icon = C_icon_validate_14;
    }
    else{
        memset(&is_selected_icon, 0, sizeof(is_selected_icon));
    }
}

void enter_keyboard_setting(uint8_t caller_id, hid_mapping_t mapping){
    if(N_storage.keyboard_layout != 0){
        nvm_write((void*)&N_storage.keyboard_layout, (void*)&mapping, sizeof(hid_mapping_t));
        display_change_keyboard_flow(change_keyboard_flow[caller_id]);
    }
    else{
        // This case only happens at application first launch
        nvm_write((void*)&N_storage.keyboard_layout, (void*)&mapping, sizeof(hid_mapping_t));
        ui_idle();
    }
}

///////////////////////////////// SETTINGS - RESET ALL PASSWORDS //////////////////////////////////////////////

void reset_password_list();

UX_STEP_CB(
reset_password_list_step,
pnn,
reset_password_list(),
{
    &C_icon_warning,
    "Reset the",
    "password list ?",
});
UX_STEP_CB(
delete_all_password_cancel_step,
pb,
ui_idle(),
{
    &C_icon_back,
    "Cancel",
});

UX_FLOW(reset_password_list_flow,
&reset_password_list_step,
&delete_all_password_cancel_step
);

void display_reset_password_list_flow(){
    ux_flow_init(0, reset_password_list_flow, NULL);
}

void reset_password_list(){
    reset_metadatas();
    ui_idle();
}

///////////////////////////////// INIT / IDLE //////////////////////////////////////////////

UX_STEP_CB(
idle_type_password_step,
pb,
display_type_password_flow(),
{
    &C_icon_bootloader,
    "Type password", 
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

UX_FLOW(idle_flow,
&idle_type_password_step,
&idle_new_password_step,
&idle_reset_password_step,
&idle_settings_step,
&idle_version_step,
&idle_quit_step,
FLOW_LOOP
);

/* Used only when the application is first launched to setup the right keyboard*/
UX_STEP_NOCB(
explanation_step,
nn,
{
    "Select the layout",
    "of your computer",
});
UX_FLOW(setup_keyboard_at_init_flow,
&explanation_step,
&qwerty_step,
&qwerty_international_step,
&azerty_step
);

void ui_idle(void) {
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    if(N_storage.keyboard_layout != 0){
        ux_flow_init(0, idle_flow, NULL);
    }
    else{
        ux_flow_init(0, setup_keyboard_at_init_flow, NULL);
    }
}
