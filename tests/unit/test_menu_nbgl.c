#include <string.h>

#include "unity.h"

#include "ui.h"
#include "ui_test_helpers.h"
#include "glyphs.h"
#include "globals.h"
#include "hid_mapping.h"
#include "options.h"
#include "password.h"
#include "status_words.h"
#include "fake_vars.h"
#include "Mockmain_std_app.h"

// Settings contents of the home page
enum { SUB_MENUS_CONTENT = 0, ENTER_CONTENT, SETTINGS_CONTENTS_NB };
// Entries of the sub-menus list
enum { CHARSET_BAR = 0, LENGTH_BAR, KBD_BAR };
// Switches of the character types sub-menu
enum { UPPERCASE_SWITCH = 0, LOWERCASE_SWITCH, NUMBERS_SWITCH, BARS_SWITCH, EXT_SYMBOLS_SWITCH };
// Entries of the action page
enum { CREATE_ACTION = 0, TYPE_ACTION, SHOW_ACTION, DELETE_ACTION, DELETE_ALL_ACTION };

static const nbgl_genericContents_t *settings;

static void check_home(uint8_t page) {
    TEST_ASSERT_EQUAL(SCREEN_HOME_AND_SETTINGS, screen.type);
    TEST_ASSERT_EQUAL(page, screen.init_page);
}

static void show_home(void) {
    ui_idle();
    check_home(INIT_HOME_PAGE);
    settings = screen.contents;
    TEST_ASSERT_EQUAL(SETTINGS_CONTENTS_NB, settings->nbContents);
}

static void open_sub_menu(uint8_t bar, int page) {
    const nbgl_content_t *bars = &settings->contentsList[SUB_MENUS_CONTENT];
    TEST_ASSERT_EQUAL(BARS_LIST, bars->type);
    bars->contentActionCallback(bars->content.barsList.tokens[bar], bar, page);
    TEST_ASSERT_EQUAL(SCREEN_GENERIC_CONFIGURATION, screen.type);
    TEST_ASSERT_EQUAL_STRING(bars->content.barsList.barTexts[bar], screen.title);
}

static const nbgl_content_t *sub_menu_content(void) {
    TEST_ASSERT_EQUAL(SCREEN_GENERIC_CONFIGURATION, screen.type);
    TEST_ASSERT_EQUAL(1, screen.contents->nbContents);
    return &screen.contents->contentsList[0];
}

static void select_choice(const nbgl_content_t *content, uint8_t index, int page) {
    TEST_ASSERT_EQUAL(CHOICES_LIST, content->type);
    content->contentActionCallback(content->content.choicesList.token, index, page);
}

static void toggle_switch(const nbgl_content_t *content, uint8_t index, int page) {
    TEST_ASSERT_EQUAL(SWITCHES_LIST, content->type);
    const nbgl_contentSwitch_t *sw = &content->content.switchesList.switches[index];
    content->contentActionCallback(sw->token, index, page);
}

static bool switch_state(const nbgl_content_t *content, uint8_t index) {
    return content->content.switchesList.switches[index].initState == ON_STATE;
}

static void check_status(const char *message, bool success) {
    TEST_ASSERT_EQUAL(SCREEN_STATUS, screen.type);
    TEST_ASSERT_EQUAL_STRING(message, screen.title);
    TEST_ASSERT_EQUAL(success, screen.success);
}

void setUp(void) {
    ui_test_setup();
    selector_callback = NULL;
    settings = NULL;
}

void tearDown(void) {
    ui_test_teardown();
}

/* ----------------------------------------------------------------------- */
/* First launch                                                            */
/* ----------------------------------------------------------------------- */

static void start_first_launch(void) {
    N_storage_rw.keyboard_layout = HID_MAPPING_NONE;
    ui_idle();
    TEST_ASSERT_EQUAL(SCREEN_CHOICE, screen.type);
    TEST_ASSERT_EQUAL_STRING("Disclaimer", screen.title);
    TEST_ASSERT_EQUAL_PTR(&C_app_passwords_64px, screen.icon);
}

static void test_first_launch_rejected(void) {
    start_first_launch();

    screen.choice(false);
    TEST_ASSERT_EQUAL(1, app_exit_calls);
    TEST_ASSERT_EQUAL(HID_MAPPING_NONE, N_storage.keyboard_layout);
}

static void test_first_launch_layout_selection(void) {
    choices_per_page = 2;
    start_first_launch();

    screen.choice(true);
    TEST_ASSERT_EQUAL_STRING("Host keyboard", screen.title);
    const nbgl_content_t *content = sub_menu_content();
    TEST_ASSERT_EQUAL(4, content->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING("AZERTY", content->content.choicesList.names[2]);

    select_choice(content, 0, 1);
    TEST_ASSERT_EQUAL(HID_MAPPING_AZERTY, N_storage.keyboard_layout);
    check_home(INIT_HOME_PAGE);
}

static void test_first_launch_layout_page_left(void) {
    start_first_launch();
    screen.choice(true);

    screen.quit();
    TEST_ASSERT_EQUAL(HID_MAPPING_QWERTY, N_storage.keyboard_layout);
    check_home(INIT_HOME_PAGE);
}

/* ----------------------------------------------------------------------- */
/* Home and action pages                                                   */
/* ----------------------------------------------------------------------- */

static void test_home_page(void) {
    show_home();
    TEST_ASSERT_EQUAL_STRING(APPNAME, screen.title);
    TEST_ASSERT_EQUAL_PTR(&C_app_passwords_64px, screen.icon);
    TEST_ASSERT_EQUAL_STRING("Tap to manage", screen.home_action->text);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.home_action->callback);
    TEST_ASSERT_EQUAL_PTR(app_exit, screen.quit);
}

// The host can replace the password page without going through its back callback
static void test_home_page_wipes_the_displayed_password(void) {
    add_password("alpha");
    display_password_list();
    show_password_cb(0);
    const char *password = sub_menu_content()->content.infosList.infoContents[0];

    ui_idle();
    TEST_ASSERT_EACH_EQUAL_CHAR('\0', password, PASSWORD_MAX_SIZE + 1);
}

static nbgl_pageContent_t action_page(void) {
    nbgl_pageContent_t content;
    memset(&content, 0, sizeof(content));

    display_choice_page();
    TEST_ASSERT_EQUAL(SCREEN_NAVIGABLE_CONTENT, screen.type);
    TEST_ASSERT_EQUAL_STRING("What do you want to do?", screen.title);
    TEST_ASSERT_TRUE(screen.nav(0, &content));
    TEST_ASSERT_EQUAL(BARS_LIST, content.type);
    return content;
}

static void select_action(uint8_t action) {
    const nbgl_pageContent_t content = action_page();
    TEST_ASSERT_TRUE(action < content.barsList.nbBars);
    screen.touch(content.barsList.tokens[action], action);
}

static void test_action_page_empty_database(void) {
    const nbgl_pageContent_t content = action_page();
    TEST_ASSERT_EQUAL(1, content.barsList.nbBars);
    TEST_ASSERT_EQUAL_STRING("Create a new password", content.barsList.barTexts[0]);
}

static void test_action_page(void) {
    add_password("alpha");

    const nbgl_pageContent_t content = action_page();
    TEST_ASSERT_EQUAL(5, content.barsList.nbBars);
    TEST_ASSERT_EQUAL_STRING("Delete all passwords", content.barsList.barTexts[DELETE_ALL_ACTION]);

    screen.quit();
    check_home(INIT_HOME_PAGE);
}

static void test_action_create(void) {
    select_action(CREATE_ACTION);
    TEST_ASSERT_EQUAL(SCREEN_KEYBOARD, screen.type);
}

static void test_action_on_a_password(void) {
    static const struct {
        uint8_t action;
        pwd_actionCallback_t callback;
    } actions[] = {
        {TYPE_ACTION, type_password_cb},
        {SHOW_ACTION, show_password_cb},
        {DELETE_ACTION, confirm_password_deletion_cb},
    };
    add_password("alpha");

    for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
        selector_callback = NULL;
        select_action(actions[i].action);
        TEST_ASSERT_EQUAL_PTR(actions[i].callback, selector_callback);
        TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
    }
}

static void test_action_delete_all(void) {
    add_password("alpha");

    select_action(DELETE_ALL_ACTION);
    TEST_ASSERT_EQUAL(SCREEN_STATIC_REVIEW, screen.type);
}

/* ----------------------------------------------------------------------- */
/* Settings: character types                                               */
/* ----------------------------------------------------------------------- */

static void test_charset_sub_menu(void) {
    show_home();

    open_sub_menu(CHARSET_BAR, 1);
    const nbgl_content_t *content = sub_menu_content();
    TEST_ASSERT_EQUAL(SWITCHES_LIST, content->type);
    TEST_ASSERT_EQUAL(5, content->content.switchesList.nbSwitches);
    TEST_ASSERT_TRUE(switch_state(content, UPPERCASE_SWITCH));
    TEST_ASSERT_TRUE(switch_state(content, LOWERCASE_SWITCH));
    TEST_ASSERT_TRUE(switch_state(content, NUMBERS_SWITCH));
    TEST_ASSERT_FALSE(switch_state(content, BARS_SWITCH));
    TEST_ASSERT_FALSE(switch_state(content, EXT_SYMBOLS_SWITCH));

    // Back to the settings page holding the entry
    screen.quit();
    check_home(1);
}

static void test_charset_toggle(void) {
    show_home();
    open_sub_menu(CHARSET_BAR, 0);
    const nbgl_content_t *content = sub_menu_content();

    toggle_switch(content, UPPERCASE_SWITCH, 0);
    TEST_ASSERT_EQUAL_HEX8(LOWERCASE_BITFLAG | NUMBERS_BITFLAG, get_charset_options());
    TEST_ASSERT_FALSE(switch_state(content, UPPERCASE_SWITCH));

    toggle_switch(content, BARS_SWITCH, 0);
    toggle_switch(content, EXT_SYMBOLS_SWITCH, 0);
    TEST_ASSERT_EQUAL_HEX8(0xFE, get_charset_options());
    TEST_ASSERT_TRUE(switch_state(content, BARS_SWITCH));
    TEST_ASSERT_TRUE(switch_state(content, EXT_SYMBOLS_SWITCH));

    toggle_switch(content, LOWERCASE_SWITCH, 0);
    toggle_switch(content, NUMBERS_SWITCH, 0);
    TEST_ASSERT_EQUAL_HEX8(BARS_BITFLAG | EXT_SYMBOLS_BITFLAG, get_charset_options());
    TEST_ASSERT_FALSE(switch_state(content, LOWERCASE_SWITCH));
    TEST_ASSERT_FALSE(switch_state(content, NUMBERS_SWITCH));
}

// Separators alone, or no set at all, are refused
static void test_charset_toggle_keeps_a_strong_set(void) {
    N_storage_rw.charset_options = LOWERCASE_BITFLAG | BARS_BITFLAG;
    show_home();
    open_sub_menu(CHARSET_BAR, 0);
    const nbgl_content_t *content = sub_menu_content();

    toggle_switch(content, LOWERCASE_SWITCH, 1);
    check_status("Keep letters, numbers\nor special characters", false);
    TEST_ASSERT_EQUAL_HEX8(LOWERCASE_BITFLAG | BARS_BITFLAG, get_charset_options());
    TEST_ASSERT_TRUE(switch_state(content, LOWERCASE_SWITCH));

    // Back to the sub-menu page of the switch
    screen.quit();
    TEST_ASSERT_EQUAL_STRING("Character types", screen.title);
    TEST_ASSERT_EQUAL(1, screen.init_page);
}

static void test_charset_toggle_keeps_pin_numbers_only(void) {
    N_storage_rw.charset_options = NUMBERS_BITFLAG;
    N_storage_rw.password_length = 4;
    show_home();
    open_sub_menu(CHARSET_BAR, 0);

    toggle_switch(sub_menu_content(), UPPERCASE_SWITCH, 0);
    check_status("PIN lengths need\nnumbers only", false);
    TEST_ASSERT_EQUAL_HEX8(NUMBERS_BITFLAG, get_charset_options());
}

static void test_enter_switch(void) {
    show_home();
    const nbgl_content_t *content = &settings->contentsList[ENTER_CONTENT];
    TEST_ASSERT_FALSE(switch_state(content, 0));

    toggle_switch(content, 0, 0);
    TEST_ASSERT_TRUE(N_storage.press_enter_after_typing);
    TEST_ASSERT_TRUE(switch_state(content, 0));

    toggle_switch(content, 0, 0);
    TEST_ASSERT_FALSE(N_storage.press_enter_after_typing);
    TEST_ASSERT_FALSE(switch_state(content, 0));
}

/* ----------------------------------------------------------------------- */
/* Settings: password length                                               */
/* ----------------------------------------------------------------------- */

static void test_length_sub_menu(void) {
    show_home();

    open_sub_menu(LENGTH_BAR, 1);
    const nbgl_content_t *content = sub_menu_content();
    TEST_ASSERT_EQUAL(10, content->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING(
        "20 characters",
        content->content.choicesList.names[content->content.choicesList.initChoice]);

    screen.quit();
    check_home(1);
}

static void test_length_selection(void) {
    show_home();
    open_sub_menu(LENGTH_BAR, 0);
    const nbgl_content_t *content = sub_menu_content();

    // Third choice of the second page
    select_choice(content, 2, 1);
    TEST_ASSERT_EQUAL(24, get_password_length());
    TEST_ASSERT_EQUAL(6, content->content.choicesList.initChoice);
}

static void test_length_selection_out_of_the_list(void) {
    show_home();
    open_sub_menu(LENGTH_BAR, 0);

    select_choice(sub_menu_content(), 2, 2);
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, get_password_length());
}

static void test_length_pin_needs_numbers_only(void) {
    show_home();
    open_sub_menu(LENGTH_BAR, 0);

    select_choice(sub_menu_content(), 0, 0);
    check_status("PIN lengths need\nnumbers only", false);
    TEST_ASSERT_EQUAL(PASSWORD_DEFAULT_SIZE, get_password_length());

    screen.quit();
    TEST_ASSERT_EQUAL_STRING("Password length", screen.title);
}

static void test_length_pin(void) {
    N_storage_rw.charset_options = NUMBERS_BITFLAG;
    show_home();
    open_sub_menu(LENGTH_BAR, 0);
    const nbgl_content_t *content = sub_menu_content();

    select_choice(content, 1, 0);
    TEST_ASSERT_EQUAL(6, get_password_length());
    TEST_ASSERT_EQUAL(1, content->content.choicesList.initChoice);
}

/* ----------------------------------------------------------------------- */
/* Settings: keyboard layout                                               */
/* ----------------------------------------------------------------------- */

static void test_kbd_sub_menu(void) {
    N_storage_rw.keyboard_layout = HID_MAPPING_AZERTY;
    show_home();

    open_sub_menu(KBD_BAR, 1);
    const nbgl_content_t *content = sub_menu_content();
    TEST_ASSERT_EQUAL_STRING(
        "AZERTY",
        content->content.choicesList.names[content->content.choicesList.initChoice]);

    screen.quit();
    check_home(1);
}

static void test_kbd_selection(void) {
    static const hid_mapping_t layouts[] = {
        HID_MAPPING_QWERTY,
        HID_MAPPING_QWERTY_INTL,
        HID_MAPPING_AZERTY,
        HID_MAPPING_QWERTZ,
    };
    show_home();
    open_sub_menu(KBD_BAR, 0);
    const nbgl_content_t *content = sub_menu_content();
    const size_t count = screen_count;

    for (uint8_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++) {
        select_choice(content, i, 0);
        TEST_ASSERT_EQUAL(layouts[i], N_storage.keyboard_layout);
        TEST_ASSERT_EQUAL(i, content->content.choicesList.initChoice);
    }
    // Out of the list
    select_choice(content, 0, 1);
    TEST_ASSERT_EQUAL(HID_MAPPING_QWERTZ, N_storage.keyboard_layout);
    TEST_ASSERT_EQUAL(3, content->content.choicesList.initChoice);
    // The sub-menu stays open
    TEST_ASSERT_EQUAL(count, screen_count);
}

/* ----------------------------------------------------------------------- */
/* User approval                                                           */
/* ----------------------------------------------------------------------- */

static void request_approval(void) {
    message_pair_t msg = {"Restore", "the passwords?"};

    ui_request_user_approval(&msg);
    TEST_ASSERT_EQUAL(SCREEN_CHOICE, screen.type);
    TEST_ASSERT_EQUAL_STRING("Restore the passwords?", screen.title);
    TEST_ASSERT_EQUAL_PTR(&C_Important_Circle_64px, screen.icon);
}

static void test_user_approval_accepted(void) {
    request_approval();

    screen.choice(true);
    TEST_ASSERT_TRUE(app_state.user_approval);
    TEST_ASSERT_EQUAL(1, dispatch_calls);
    TEST_ASSERT_EQUAL(0, sw_count);
    check_home(INIT_HOME_PAGE);
}

static void test_user_approval_refused(void) {
    request_approval();

    screen.choice(false);
    TEST_ASSERT_FALSE(app_state.user_approval);
    TEST_ASSERT_EQUAL(0, dispatch_calls);
    TEST_ASSERT_EQUAL(1, sw_count);
    TEST_ASSERT_EQUAL_HEX16(SWO_CONDITIONS_NOT_SATISFIED, last_sw);
    check_home(INIT_HOME_PAGE);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_first_launch_rejected);
    RUN_TEST(test_first_launch_layout_selection);
    RUN_TEST(test_first_launch_layout_page_left);
    RUN_TEST(test_home_page);
    RUN_TEST(test_home_page_wipes_the_displayed_password);
    RUN_TEST(test_action_page_empty_database);
    RUN_TEST(test_action_page);
    RUN_TEST(test_action_create);
    RUN_TEST(test_action_on_a_password);
    RUN_TEST(test_action_delete_all);
    RUN_TEST(test_charset_sub_menu);
    RUN_TEST(test_charset_toggle);
    RUN_TEST(test_charset_toggle_keeps_a_strong_set);
    RUN_TEST(test_charset_toggle_keeps_pin_numbers_only);
    RUN_TEST(test_enter_switch);
    RUN_TEST(test_length_sub_menu);
    RUN_TEST(test_length_selection);
    RUN_TEST(test_length_selection_out_of_the_list);
    RUN_TEST(test_length_pin_needs_numbers_only);
    RUN_TEST(test_length_pin);
    RUN_TEST(test_kbd_sub_menu);
    RUN_TEST(test_kbd_selection);
    RUN_TEST(test_user_approval_accepted);
    RUN_TEST(test_user_approval_refused);
    return UNITY_END();
}
