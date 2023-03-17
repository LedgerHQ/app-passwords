/*******************************************************************************
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
 ********************************************************************************/

#include <os.h>
#include <string.h>

#if defined(TARGET_STAX)

#include <nbgl_layout.h>
#include <nbgl_page.h>
#include <nbgl_use_case.h>

#include "../error.h"
#include "../globals.h"
#include "../metadata.h"
#include "../options.h"
#include "../password.h"
#include "../dispatcher.h"

#include "glyphs.h"
#include "password_list.h"
#include "ui.h"

static nbgl_page_t *pageContext;
static nbgl_layout_t *layoutContext = 0;
#define MAX_ERROR_MSG_SIZE 100
static char errorMessage[MAX_ERROR_MSG_SIZE] = {0};

enum {
    BACK_BUTTON_TOKEN = FIRST_USER_TOKEN,
    CREATE_TOKEN,
    UPPERCASE_TOKEN,
    LOWERCASE_TOKEN,
    NUMBERS_TOKEN,
    BARS_TOKEN,
    EXT_SYMBOLS_TOKEN,
    ERASE_METADATA_TOKEN,
    NO_ENTER_TOKEN,
    CHOICE_WRITE_TOKEN,
    CHOICE_DISPLAY_TOKEN,
    CHOICE_CREATE_TOKEN,
    CHOICE_DELETE_TOKEN,
    KBD_TEXT_TOKEN,
};

static void release_context(void) {
    if (pageContext != NULL) {
        nbgl_pageRelease(pageContext);
        pageContext = NULL;
    }
    if (layoutContext != NULL) {
        nbgl_layoutRelease(layoutContext);
        layoutContext = NULL;
    }
}

static void display_choice_page(void);
static void display_create_pwd_page(void);
static void display_settings_page(void);

/*
 * Error display
 */
static void display_error_page(error_type_t error) {
    message_pair_t msg = get_error(error);
    const size_t msg_length = msg.first_len + msg.second_len + 2;
    snprintf(&errorMessage[0],
             msg_length > MAX_ERROR_MSG_SIZE ? MAX_ERROR_MSG_SIZE : msg_length,
             "%s\n%s",
             (char *) PIC(msg.first),
             (char *) PIC(msg.second));
    nbgl_useCaseStatus(&errorMessage[0], false, &display_choice_page);
}

static void display_success_page(char *string) {
    nbgl_useCaseStatus(string, true, &display_choice_page);
}

/*
 * Settings menu
 */
static const char *const infoTypes[] = {"Version", "Passwords"};
static const char *const infoContents[] = {APPVERSION, "(c) 2017-2023 Ledger"};

#define SETTINGS_CHARSET_OPTIONS_NUMBER 5
#define SETTINGS_MISC_OPTIONS_NUMBER    1
#define SETTINGS_INFO_NUMBER            2
#define SETTINGS_PAGE_NUMBER            3

static nbgl_layoutSwitch_t switches[SETTINGS_CHARSET_OPTIONS_NUMBER];

static bool display_settings_navigation(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        switches[0] = (nbgl_layoutSwitch_t){.initState = has_charset_option(UPPERCASE_BITFLAG),
                                            .text = "Use uppercase",
                                            .subText = NULL,
                                            .token = UPPERCASE_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[1] = (nbgl_layoutSwitch_t){.initState = has_charset_option(LOWERCASE_BITFLAG),
                                            .text = "Use lowecase",
                                            .subText = NULL,
                                            .token = LOWERCASE_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[2] = (nbgl_layoutSwitch_t){.initState = has_charset_option(NUMBERS_BITFLAG),
                                            .text = "Use numbers",
                                            .subText = NULL,
                                            .token = NUMBERS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[3] = (nbgl_layoutSwitch_t){.initState = has_charset_option(BARS_BITFLAG),
                                            .text = "Use separators",
                                            .subText = "('-', ' ', '_')",
                                            .token = BARS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[4] = (nbgl_layoutSwitch_t){.initState = has_charset_option(EXT_SYMBOLS_BITFLAG),
                                            .text = "Use special characters",
                                            .subText = NULL,
                                            .token = EXT_SYMBOLS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = SETTINGS_CHARSET_OPTIONS_NUMBER;
        content->switchesList.switches = &switches[0];
    } else if (page == 1) {
        switches[0] = (nbgl_layoutSwitch_t){.initState = N_storage.press_enter_after_typing,
                                            .text = "Press enter",
                                            .subText = "after writing the password",
                                            .token = NO_ENTER_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = SETTINGS_MISC_OPTIONS_NUMBER;
        content->switchesList.switches = &switches[0];
    } else if (page == 2) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = SETTINGS_INFO_NUMBER;
        content->infosList.infoTypes = (const char **) infoTypes;
        content->infosList.infoContents = (const char **) infoContents;
    } else {
        return false;
    }
    return true;
}

static void charset_settings_callback(const int token,
                                      const uint8_t index __attribute__((unused))) {
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
        default:
            break;
    }
}

static void check_settings_before_home() {
    if (get_charset_options() == 0) {
        nbgl_useCaseStatus("At least one charset\nmust be selected", false, &display_settings_page);
    } else {
        display_home_page();
    }
}

static void display_settings_page() {
    nbgl_useCaseSettings("Password settings",
                         0,
                         SETTINGS_PAGE_NUMBER,
                         false,
                         check_settings_before_home,
                         display_settings_navigation,
                         charset_settings_callback);
}

/*
 * Password creation & callback
 */
static char password_name[MAX_METANAME + 1] = {0};
static int textIndex, keyboardIndex = 0;

static void create_password(void) {
    PRINTF("Creating new password '%s'\n", password_name);
    release_context();
    const size_t password_size = strlen(password_name);
    if (password_size == 0) {
        nbgl_useCaseStatus("The nickname\ncan't be empty", false, &display_create_pwd_page);
    } else {
        error_type_t error = create_new_password(password_name, password_size);
        if (error == OK) {
            display_success_page("NEW PASSWORD\nCREATED");
            // nbgl_useCaseStatus("NEW PASSWORD\nCREATED", true, &display_choice_page);
        } else {
            PRINTF("The error nb %d occurred\n", error);
            display_error_page(error);
        }
    }
}

static void page_callback(const int token, const uint8_t index __attribute__((unused))) {
    switch (token) {
        case BACK_BUTTON_TOKEN:
            display_choice_page();
            break;
        case CREATE_TOKEN:
            create_password();
            break;
        default:
            break;
    }
};

static void key_press_callback(const char touchedKey) {
    size_t textLen = 0;
    uint32_t mask = 0;
    const size_t previousTextLen = strlen(password_name);
    if (touchedKey == BACKSPACE_KEY) {
        if (previousTextLen == 0) {
            return;
        }
        password_name[previousTextLen - 1] = '\0';
        textLen = previousTextLen - 1;
    } else {
        password_name[previousTextLen] = touchedKey;
        password_name[previousTextLen + 1] = '\0';
        textLen = previousTextLen + 1;
    }
    if (textLen >= MAX_METANAME) {
        // password name length can't be greater than MAX_METANAME, so we mask
        // every characters
        mask = -1;
    }
    nbgl_layoutUpdateKeyboard(layoutContext, keyboardIndex, mask, false, LOWER_CASE);
    nbgl_layoutUpdateEnteredText(layoutContext, textIndex, false, 0, &(password_name[0]), false);
    nbgl_refresh();
}

static void display_create_pwd_page() {
    release_context();
    nbgl_layoutDescription_t layoutDescription = {.modal = false,
                                                  .onActionCallback = &page_callback};
    nbgl_layoutKbd_t kbdInfo = {.lettersOnly = false,
                                .mode = MODE_LETTERS,
                                .keyMask = 0,
                                .callback = &key_press_callback};
    nbgl_layoutCenteredInfo_t centeredInfo = {.text1 = NULL,
                                              .text2 = "New password nickname",
                                              .text3 = NULL,
                                              .style = LARGE_CASE_INFO,
                                              .icon = NULL,
                                              .offsetY = 0,
                                              .onTop = true};
    layoutContext = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddProgressIndicator(layoutContext, 0, 0, true, BACK_BUTTON_TOKEN, TUNE_TAP_CASUAL);
    nbgl_layoutAddCenteredInfo(layoutContext, &centeredInfo);
    keyboardIndex = nbgl_layoutAddKeyboard(layoutContext, &kbdInfo);
    nbgl_layoutAddConfirmationButton(layoutContext,
                                     true,
                                     "Create password",
                                     CREATE_TOKEN,
                                     TUNE_TAP_CASUAL);
    strlcpy(password_name, "", 1);
    textIndex = nbgl_layoutAddEnteredText(layoutContext,
                                          false,
                                          0,
                                          password_name,
                                          false,
                                          32,
                                          KBD_TEXT_TOKEN);
    nbgl_layoutDraw(layoutContext);
}

/*
 * Password list
 * Used to go to either print, display or delete
 */

/* Callback to trigger with password offset - print, display or delete */
static void (*selector_callback)(const size_t index);

static bool display_password_list_navigation(uint8_t page, nbgl_pageContent_t *content) {
    size_t passwordIndex = page * DISPLAYED_PASSWORD_PER_PAGE;
    password_list_reset_buffer();
    size_t localIndex = 0;
    while (localIndex < DISPLAYED_PASSWORD_PER_PAGE && passwordIndex < N_storage.metadata_count) {
        size_t pwdOffset = get_metadata(passwordIndex);
        if (pwdOffset == -1UL) {
            break;
        }
        const size_t pwdLength = METADATA_NICKNAME_LEN(pwdOffset) + 1;
        password_list_add_password(localIndex,
                                   pwdOffset,
                                   (void *) METADATA_NICKNAME(pwdOffset),
                                   pwdLength);
        localIndex++;
        passwordIndex++;
    }

    content->type = CHOICES_LIST;
    content->choicesList.names = password_list_passwords();
    content->choicesList.localized = false;
    content->choicesList.nbChoices = localIndex;
    content->choicesList.initChoice = 0;
    content->choicesList.token = -1;
    content->choicesList.tuneId = TUNE_TAP_CASUAL;
    return true;
}

static void password_list_callback(const int token __attribute__((unused)), const uint8_t index) {
    selector_callback(index);
}

static void display_password_list_page() {
    password_list_reset();
    nbgl_useCaseSettings("Password list",
                         0,
                         2,
                         false,
                         display_choice_page,
                         display_password_list_navigation,
                         password_list_callback);
}

/*
 * Display single password page & dispatcher
 */
static char password_to_display[PASSWORD_MAX_SIZE + 1] = {0};
static const char *ptrToPwd[2] = {0};

static bool display_password_navigation(uint8_t page __attribute__((unused)),
                                        nbgl_pageContent_t *content) {
    content->type = INFOS_LIST;
    content->infosList.nbInfos = 1;
    content->infosList.infoTypes = (const char **) &ptrToPwd[0];
    content->infosList.infoContents = (const char **) &ptrToPwd[1];
    return true;
}

static void display_password_page() {
    nbgl_useCaseSettings("Your password:",
                         0,
                         1,
                         false,
                         display_choice_page,
                         display_password_navigation,
                         NULL);
}

/*
 * Password management callbacks
 */

void type_password_cb(const size_t index) {
    type_password_at_offset(password_list_get_offset(index));
    display_success_page("PASSWORD HAS\nBEEN WRITTEN");
}

void show_password_cb(const size_t index) {
    ptrToPwd[0] = password_list_get_password(index);
    show_password_at_offset(password_list_get_offset(index), (uint8_t *) password_to_display);
    ptrToPwd[1] = &password_to_display[0];
    display_password_page();
}

/*
 * Delete confirmation pages
 */

static const char deletionPrefix[] = "Confirm the deletion\n";
/* '\n' actually takes only 1 char */
static const int deletionPrefixSize = sizeof(deletionPrefix) - 1;  // size 21

static const char oneDeletion[] = "of password\n'%s'";
static const char allDeletion[] = "of all passwords (%d)";

/*
 * This buffer will hold the string obtained by formatting and concatenated `deletionPrefix` and
 * (`oneDeletion` OR `allDeletion`).
 * - `deletionPrefix` is 21 chars
 * - `oneDeletion` is 17 chars, but formatted \n counts for 1, not 2 (-1) and %s will be replaced
 *   (-1) by a string which length is at most MAX_METANAME (20). So 17 - 1 - 2 + 20 = 34
 * - `allDeletion` is 21 chars, but %d will be replaced by a string which length is at most
 *   sizeof(STR(MAX_METADATAS)) = sizeof("4096") = 4. So 21 - 2 + 4 = 23
 * So the total buffer length is 21 + max(34, 23) + 1 terminating null byte = 56
 */
#define DELETION_BUFFER_SIZE (21 + 34 + 1)
static char deletionBuffer[DELETION_BUFFER_SIZE] = {0};
static nbgl_layoutTagValueList_t pairList = {.nbMaxLinesForValue = 0, .nbPairs = 0, .pairs = NULL};

static void write_deletion_message(const size_t index) {
    snprintf(&deletionBuffer[0], DELETION_BUFFER_SIZE, deletionPrefix);
    if (index == (size_t) -1) {
        // deleting all passwords
        snprintf(&deletionBuffer[0] + deletionPrefixSize,
                 DELETION_BUFFER_SIZE - deletionPrefixSize,
                 allDeletion,
                 N_storage.metadata_count);
    } else {
        snprintf(&deletionBuffer[0] + deletionPrefixSize,
                 DELETION_BUFFER_SIZE - deletionPrefixSize,
                 oneDeletion,
                 password_list_get_password(index));
    }
}

static void delete_password(const bool all) {
    if (all) {
        reset_metadatas();
        display_success_page("PASSWORDS HAVE\nBEEN DELETED");
        return;
    }
    const error_type_t result = delete_password_at_offset(password_list_get_current_offset());
    if (result == OK) {
        display_success_page("PASSWORD HAS\nBEEN DELETED");
    } else {
        display_error_page(result);
    }
}

static void reviewDeletePasswordChoice(bool confirm) {
    if (confirm) {
        delete_password(false);
    } else {
        display_choice_page();
    }
}

static void reviewDeletePasswordsChoice(bool confirm) {
    if (confirm) {
        delete_password(true);
    } else {
        display_choice_page();
    }
}

static void confirm_password_deletion_cb(const size_t index) {
    write_deletion_message(index);
    nbgl_pageInfoLongPress_t infoLongPress = {.icon = NULL,
                                              .text = &deletionBuffer[0],
                                              .longPressText = "Hold to confirm"};
    password_list_set_current(index);
    nbgl_useCaseStaticReview(&pairList,
                             &infoLongPress,
                             "Don't remove the password",
                             reviewDeletePasswordChoice);
}

static void confirm_all_passwords_deletion() {
    write_deletion_message(-1);
    nbgl_pageInfoLongPress_t infoLongPress = {.icon = NULL,
                                              .text = &deletionBuffer[0],
                                              .longPressText = "Hold to confirm"};
    nbgl_useCaseStaticReview(&pairList,
                             &infoLongPress,
                             "Don't remove\nall the passwords",
                             reviewDeletePasswordsChoice);
}

/*
 * Choice page (create, print, display, delete) & dispatcher
 */

static const char *const bars[] = {"Type a password",
                                   "Show a password",
                                   "Create a new password",
                                   "Delete a password",
                                   "Delete all passwords"};
static const uint8_t barsToken[] = {CHOICE_WRITE_TOKEN,
                                    CHOICE_DISPLAY_TOKEN,
                                    CHOICE_CREATE_TOKEN,
                                    CHOICE_DELETE_TOKEN,
                                    ERASE_METADATA_TOKEN};

static bool choice_navigation_callback(const uint8_t page __attribute__((unused)),
                                       nbgl_pageContent_t *content) {
    content->type = BARS_LIST;
    content->barsList.nbBars = 5;
    content->barsList.barTexts = (const char **const) bars;
    content->barsList.tokens = (uint8_t *const) barsToken;
    return true;
}

static void choice_callback(const int token, const uint8_t index __attribute__((unused))) {
    switch (token) {
        case CHOICE_WRITE_TOKEN:
            selector_callback = type_password_cb;
            display_password_list_page();
            break;
        case CHOICE_DISPLAY_TOKEN:
            selector_callback = show_password_cb;
            display_password_list_page();
            break;
        case CHOICE_CREATE_TOKEN:
            display_create_pwd_page();
            break;
        case CHOICE_DELETE_TOKEN:
            selector_callback = confirm_password_deletion_cb;
            display_password_list_page();
            break;
        case ERASE_METADATA_TOKEN:
            confirm_all_passwords_deletion();
            break;
    }
}

static void display_choice_page() {
    release_context();
    nbgl_useCaseSettings("What do you want to do?",
                         0,      // first page index
                         1,      // page number
                         false,  // touchable title
                         display_home_page,
                         choice_navigation_callback,
                         choice_callback);
}

/*
 * Home page
 */
static void quit() {
    release_context();
    os_sched_exit(-1);
}

void display_home_page(void) {
    nbgl_useCaseHomeExt("Passwords",
                        &C_stax_icon_password_manager_64px,
                        "Create, type and display\npasswords through\nyour device",
                        true,
                        "Tap to manage\nyour passwords",
                        display_choice_page,
                        display_settings_page,
                        quit);
}

/*
 * Approval page
 */
void approval_granted() {
    app_state.user_approval = true;
    dispatch();
    display_home_page();
}

void display_approval_page(message_pair_t *msg) {
    // using errorMessage to store the message to display
    const size_t msgLen = msg->first_len + msg->second_len + 1;
    snprintf(&errorMessage[0],
             msgLen,
             "%s %s",
             (char *) PIC(msg->first),
             (char *) PIC(msg->second));
    PRINTF("Waiting confirmation: '%s'\n", &errorMessage[0]);
    nbgl_useCaseConfirm(&errorMessage[0], NULL, "Approve", "Refuse", approval_granted);
}

#endif
