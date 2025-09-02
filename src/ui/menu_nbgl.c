
/*****************************************************************************
 *   Password Manager application
 *   (c) 2017-2023 Ledger SAS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include "os.h"
#include "glyphs.h"
#include "nbgl_use_case.h"
#include "main_std_app.h"

#include "ui.h"
#include "dispatcher.h"
#include "globals.h"
#include "options.h"

// #define WITH_KBD_MAPPING

#define MAX_ERROR_MSG_SIZE 100
static char errorMessage[MAX_ERROR_MSG_SIZE] = {0};

// clang-format off

/**
 * @brief Token to catch events in UI callbacks
 *
 */
enum {
    UPPERCASE_TOKEN = FIRST_USER_TOKEN,
    LOWERCASE_TOKEN,
    NUMBERS_TOKEN,
    BARS_TOKEN,
    EXT_SYMBOLS_TOKEN,
    NO_ENTER_TOKEN,
    KBD_TOKEN,
    CHOICE_WRITE_TOKEN,
    CHOICE_DISPLAY_TOKEN,
    CHOICE_CREATE_TOKEN,
    CHOICE_DELETE_TOKEN,
    CHOICE_DELETE_ALL_TOKENS,
};

/**
 * @brief Settings: Switches IDs
 *
 */
enum {
    UPPERCASE_ID = 0,
    LOWERCASE_ID,
    NUMBERS_ID,
    BARS_ID,
    EXT_SYMBOLS_ID,
    NO_ENTER_ID,
    SETTINGS_SWITCHES_NB,
};

/**
 * @brief Settings: Keyboard layout IDs
 *
 */
enum {
    KBD_QWERTY_ID = 0,
    KBD_QWERTY_INTL_ID,
    KBD_AZERTY_ID,
    KBD_OPTIONS_NB,
};

/**
 * @brief Settings: contents indexes
 *
 */
enum {
    SETTING_OPTIONS,
#ifdef WITH_KBD_MAPPING
    SETTING_KBD_TYPE,
#endif
    SETTING_CONTENTS_NB,
};

static void display_home_page(void);

// settings init display page index (allows to target keyboard layout)
uint8_t initSettingPage;

// Settings switches
static nbgl_contentSwitch_t switches[SETTINGS_SWITCHES_NB] = {0};

#ifdef WITH_KBD_MAPPING
// Settings keyboard layouts
static const char *const availableKbd[KBD_OPTIONS_NB] = {"QWERTY", "QWERTY INT.", "AZERTY"};
bool kbdMappingInit = false;
#endif

// App info
#define SETTING_INFO_NB 2
static const char* const INFO_TYPES[SETTING_INFO_NB] = {"Version", "Developer"};
static const char* const INFO_CONTENTS[SETTING_INFO_NB] = {APPVERSION, "(c) 2017-2025 Ledger"};
static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = INFO_TYPES,
    .infoContents = INFO_CONTENTS,
};

// Home and settings content definitions
static nbgl_content_t contents[SETTING_CONTENTS_NB] = {0};
static const nbgl_genericContents_t settingContents = {
    .callbackCallNeeded = false,
    .contentsList = contents,
    .nbContents = SETTING_CONTENTS_NB
};
static nbgl_homeAction_t homeAction = {0};

// Main Action menu choices
#define ACTIONS_NB 5
static const char *const barsTexts[ACTIONS_NB] = {
    "Type a password",
    "Show a password",
    "Create a new password",
    "Delete a password",
    "Delete all passwords"
};
static const uint8_t barsToken[ACTIONS_NB] = {
    CHOICE_WRITE_TOKEN,
    CHOICE_DISPLAY_TOKEN,
    CHOICE_CREATE_TOKEN,
    CHOICE_DELETE_TOKEN,
    CHOICE_DELETE_ALL_TOKENS
};

// Callback to trigger with password offset - print, display or delete
pwd_actionCallback_t selector_callback = NULL;

// clang-format on

/**
 * @brief Settings Action callback
 *
 * @param[in] token button Id pressed
 * @param[in] index widget index on the page
 * @param[in] page index of the page
 *
 */
static void controls_callback(int token, uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);

    switch (token) {
        case UPPERCASE_TOKEN:
            set_charset_option(UPPERCASE_BITFLAG);
            break;
        case LOWERCASE_TOKEN:
            set_charset_option(LOWERCASE_BITFLAG);
            break;
        case NUMBERS_TOKEN:
            set_charset_option(NUMBERS_BITFLAG);
            break;
        case BARS_TOKEN:
            set_charset_option(BARS_BITFLAG);
            break;
        case EXT_SYMBOLS_TOKEN:
            set_charset_option(EXT_SYMBOLS_BITFLAG);
            break;
        case NO_ENTER_TOKEN:
            change_enter_options();
            break;
#ifdef WITH_KBD_MAPPING
        case KBD_TOKEN:
            switch (index) {
                case KBD_QWERTY_ID:
                    set_keyboard_layout(HID_MAPPING_QWERTY);
                    break;
                case KBD_QWERTY_INTL_ID:
                    set_keyboard_layout(HID_MAPPING_QWERTY_INTL);
                    break;
                case KBD_AZERTY_ID:
                    set_keyboard_layout(HID_MAPPING_AZERTY);
                    break;
                default:
                    break;
            }
            contents[SETTING_KBD_TYPE].content.choicesList.initChoice = index;
            if (kbdMappingInit) {
                kbdMappingInit = false;
                display_home_page();
            }
            break;
#endif  // WITH_KBD_MAPPING
        default:
            break;
    }
}

/**
 * @brief Init settings parameters
 *
 */
static void init_settings(void) {
    // Initialize switches data
    // ------------------------
    switches[UPPERCASE_ID].initState = has_charset_option(UPPERCASE_BITFLAG);
    switches[UPPERCASE_ID].text = "Use uppercase";
    switches[UPPERCASE_ID].token = UPPERCASE_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[UPPERCASE_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    switches[LOWERCASE_ID].initState = has_charset_option(LOWERCASE_BITFLAG);
    switches[LOWERCASE_ID].text = "Use lowercase";
    switches[LOWERCASE_ID].token = LOWERCASE_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[LOWERCASE_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    switches[NUMBERS_ID].initState = has_charset_option(NUMBERS_BITFLAG);
    switches[NUMBERS_ID].text = "Use numbers";
    switches[NUMBERS_ID].token = NUMBERS_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[NUMBERS_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    switches[BARS_ID].initState = has_charset_option(BARS_BITFLAG);
    switches[BARS_ID].text = "Use separators";
    switches[BARS_ID].subText = "('-', ' ', '_')";
    switches[BARS_ID].token = BARS_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[BARS_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    switches[EXT_SYMBOLS_ID].initState = has_charset_option(EXT_SYMBOLS_BITFLAG);
#ifdef SCREEN_SIZE_WALLET
    switches[EXT_SYMBOLS_ID].text = "Use special characters";
#else
    switches[EXT_SYMBOLS_ID].text = "Use special chars";
#endif
    switches[EXT_SYMBOLS_ID].token = EXT_SYMBOLS_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[EXT_SYMBOLS_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    switches[NO_ENTER_ID].initState = N_storage.press_enter_after_typing;
    switches[NO_ENTER_ID].text = "Press enter";
    switches[NO_ENTER_ID].subText = "after writing the password";
    switches[NO_ENTER_ID].token = NO_ENTER_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    switches[NO_ENTER_ID].tuneId = TUNE_TAP_CASUAL;
#endif

    // Initialize settings content
    // ---------------------------
    contents[SETTING_OPTIONS].type = SWITCHES_LIST;
    contents[SETTING_OPTIONS].content.switchesList.nbSwitches = SETTINGS_SWITCHES_NB;
    contents[SETTING_OPTIONS].content.switchesList.switches = switches;
    contents[SETTING_OPTIONS].contentActionCallback = controls_callback;

#ifdef WITH_KBD_MAPPING
    contents[SETTING_KBD_TYPE].type = CHOICES_LIST;
    contents[SETTING_KBD_TYPE].content.choicesList.nbChoices = KBD_OPTIONS_NB;
    contents[SETTING_KBD_TYPE].content.choicesList.names = availableKbd;
    contents[SETTING_KBD_TYPE].content.choicesList.token = KBD_TOKEN;
    contents[SETTING_KBD_TYPE].contentActionCallback = controls_callback;
#ifdef HAVE_PIEZO_SOUND
    contents[SETTING_KBD_TYPE].content.choicesList.tuneId = TUNE_TAP_CASUAL;
#endif

    if (N_storage.keyboard_layout != HID_MAPPING_NONE) {
        contents[SETTING_KBD_TYPE].content.choicesList.initChoice = N_storage.keyboard_layout - 1;
    }
#endif  // WITH_KBD_MAPPING

    // Initialize Home page Action
    // ---------------------------
    homeAction.text = "Tap to manage";
    homeAction.callback = display_choice_page;
}

/**
 * @brief Home page Action control callback
 *
 * @param[in] token button Id pressed
 * @param[in] index widget index on the page
 *
 */
static void choice_callback(const int token, const uint8_t index) {
    UNUSED(index);
    switch (token) {
        case CHOICE_WRITE_TOKEN:
            selector_callback = type_password_cb;
            display_password_list();
            break;
        case CHOICE_DISPLAY_TOKEN:
            selector_callback = show_password_cb;
            display_password_list();
            break;
        case CHOICE_CREATE_TOKEN:
            display_create_pwd();
            break;
        case CHOICE_DELETE_TOKEN:
            selector_callback = confirm_password_deletion_cb;
            display_password_list();
            break;
        case CHOICE_DELETE_ALL_TOKENS:
            confirm_all_passwords_deletion();
            break;
    }
}

/**
 * @brief Home page Action navigation callback
 *
 * @param[in] page index of the page
 * @param[out] content pointer to the content structure
 * @return true if the navigation was successful, false otherwise
 *
 */
static bool choice_navigation_callback(const uint8_t page, nbgl_pageContent_t *content) {
    UNUSED(page);
    content->type = BARS_LIST;
    content->barsList.nbBars = ACTIONS_NB;
    content->barsList.barTexts = barsTexts;
    content->barsList.tokens = barsToken;
#ifdef HAVE_PIEZO_SOUND
    content->barsList.tuneId = TUNE_TAP_CASUAL;
#endif
    return true;
}

/**
 * @brief Home page Action callback
 *
 */
void display_choice_page(void) {
    nbgl_useCaseNavigableContent(
#ifdef SCREEN_SIZE_WALLET
        "What do you want to do?",
#else
        "Which action?",
#endif
        0,
        1,
        display_home_page,
        choice_navigation_callback,
        choice_callback);
}

/**
 * @brief Display the App Home page
 *
 */
static void display_home_page(void) {
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &ICON_APP_HOME,
#ifdef SCREEN_SIZE_WALLET
                                "Create, type and display\npasswords through\nyour device",
#else
                                "Manage passwords on your device",
#endif
                                initSettingPage,
                                &settingContents,
                                &infoList,
                                &homeAction,
                                app_exit);
}

/**
 * @brief UI startup callback
 * @note At 1st init of the app, it allows the user to select the keyboard type
 *
 * @param[in] confirm if the user accepts the disclaimer
 *
 */
static void startup_callback(bool confirm) {
#ifdef WITH_KBD_MAPPING
    uint8_t index = 0;
#endif

    if (confirm) {
#ifdef WITH_KBD_MAPPING
        kbdMappingInit = true;
        initSettingPage = 0;
        // Find the page index after the switches, where the settings show the keyboard type
        while (index < SETTINGS_SWITCHES_NB) {
            index +=
                nbgl_useCaseGetNbSwitchesInPage(SETTINGS_SWITCHES_NB - index,
                                                &contents[SETTING_OPTIONS].content.switchesList,
                                                index,
                                                false);
            initSettingPage++;
        }
#endif  // WITH_KBD_MAPPING
        display_home_page();
        initSettingPage = INIT_HOME_PAGE;
    } else {
        app_exit();
    }
}

/**
 * @brief UI entry point
 *
 */
void ui_idle(void) {
    initSettingPage = INIT_HOME_PAGE;
    init_settings();
    // First start: the keyboard layout is not selected yet
    if (N_storage.keyboard_layout == HID_MAPPING_NONE) {
        nbgl_useCaseChoice(&ICON_APP_HOME,
                           "Disclaimer",
                           "Backup your passwords before any update: "
                           "\nhttps://passwords.ledger.com\n\nIf not, they "
                           "will be lost.",
                           "Yes, I understand",
                           "No, this is too complicated",
                           startup_callback);
    } else {
        display_home_page();
    }
}

/**
 * @brief Approval callback
 *
 */
static void approval_granted(void) {
    app_state.user_approval = true;
    dispatch();
    display_home_page();
}

/**
 * @brief Request user approval
 *
 * @param[in] msg message to display
 *
 */
void ui_request_user_approval(message_pair_t *msg) {
    // using errorMessage to store the message to display
    snprintf(&errorMessage[0],
             sizeof(errorMessage),
             "%s %s",
             (char *) PIC(msg->first),
             (char *) PIC(msg->second));
    nbgl_useCaseConfirm(&errorMessage[0], NULL, "Approve", "Refuse", approval_granted);
}
