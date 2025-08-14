
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

#include "error.h"
#include "ui.h"
#include "password.h"
#include "password_list.h"
#include "dispatcher.h"
#include "globals.h"
#include "options.h"
#include "metadata.h"

/**
 * @brief Token to catch events in UI callbacks
 *
 */
enum {
    BACK_BUTTON_TOKEN = FIRST_USER_TOKEN,
    CREATE_TOKEN,
    KBD_TEXT_TOKEN,
};

#define MAX_MSG_BUFFER_SIZE 100
static char msgBuffer[MAX_MSG_BUFFER_SIZE] = {0};

static char password_to_display[PASSWORD_MAX_SIZE + 1] = {0};
static const char *ptrToPwd[2] = {0};

// Pointer to the layout context
static nbgl_layout_t *layoutContext = NULL;

// Flag to indicate if all passwords are being deleted
static bool all_passwords;

// Keyboard contexts
static char password_name[MAX_METANAME + 1] = {0};
static int keyboardIndex = 0;

static nbgl_layoutConfirmationButton_t confirmButton = {0};
static nbgl_layoutKeyboardContent_t keyboardContent = {0};

/**
 * @brief Delete error message
 *
 * @param[in] error type
 *
 */
static void display_error_page(error_type_t error) {
    message_pair_t msg = get_error(error);
    snprintf(&msgBuffer[0],
             sizeof(msgBuffer),
             "%s\n%s",
             (char *) PIC(msg.first),
             (char *) PIC(msg.second));
    nbgl_useCaseStatus(&msgBuffer[0], false, display_choice_page);
}

/**
 * @brief Display success message
 *
 * @param[in] string message
 *
 */
static void display_success_page(const char *string) {
    nbgl_useCaseStatus(string, true, display_choice_page);
}

/**
 * @brief Cleanup UI context
 *
 */
static void release_context(void) {
    if (layoutContext != NULL) {
        nbgl_layoutRelease(layoutContext);
        layoutContext = NULL;
    }
}

/**
 * @brief Delete password
 *
 * @param[in] all if true, delete all passwords
 *
 */
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

/**
 * @brief Delete password confirmation callback
 *
 * @param[in] confirm if the user accepts the disclaimer
 *
 */
static void reviewDeletePassword_callback(bool confirm) {
    if (confirm) {
        delete_password(all_passwords);
    } else {
        display_choice_page();
    }
}

/**
 * @brief Confirmation to delete passwords
 *
 * @param[in] index of the password
 *
 */
static void confirm_password_deletion(const size_t index) {
    nbgl_contentInfoLongPress_t infoLongPress = {.icon = NULL,
                                                 .text = msgBuffer,
                                                 .longPressText = "Hold to confirm"};

    if (index == -1UL) {
        // All passwords
        all_passwords = true;
        snprintf(msgBuffer,
                 sizeof(msgBuffer),
                 "Confirm the deletion\nof all passwords (%d)",
                 N_storage.metadata_count);
    } else {
        // A single password
        all_passwords = false;
        snprintf(msgBuffer,
                 sizeof(msgBuffer),
                 "Confirm the deletion\nof password\n'%s'",
                 password_list_get_password(index));
        password_list_set_current(index);
    }
    nbgl_useCaseStaticReview(NULL,
                             &infoLongPress,
                             NULL,
                             reviewDeletePassword_callback);
}

/**
 * @brief Confirmation to delete one password
 *
 * @param[in] index of the password
 *
 */
void confirm_password_deletion_cb(const size_t index) {
    confirm_password_deletion(index);
}

/**
 * @brief Confirmation to delete all passwords
 *
 */
void confirm_all_passwords_deletion(void) {
    confirm_password_deletion(-1);
}

/**
 * @brief Passwords control callback
 *
 * @param[in] token button Id pressed
 * @param[in] index widget index on the page
 *
 */
static void password_callback(const int token, const uint8_t index) {
    UNUSED(index);
    UNUSED(token);
    if (selector_callback) {
        selector_callback(index);
    }
}

/**
 * @brief Passwords navigation callback
 *
 * @param[in] page index of the page
 * @param[out] content pointer to the content structure
 * @return true if the navigation was successful, false otherwise
 *
 */
static bool passwords_list_callback(const uint8_t page, nbgl_pageContent_t *content) {
    UNUSED(page);
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
#ifdef HAVE_PIEZO_SOUND
    content->choicesList.tuneId = TUNE_TAP_CASUAL;
#endif
    return true;
}

/**
 * @brief Display the passwords list
 *
 */
void display_password_list(void) {
    password_list_reset();
    nbgl_useCaseNavigableContent("Passwords list",
                                 0,
                                 1,
                                 display_choice_page,
                                 passwords_list_callback,
                                 password_callback);
}

/**
 * @brief Password show callback
 *
 * @param[in] page index of the page
 * @param[out] content pointer to the content structure
 * @return true if the navigation was successful, false otherwise
 *
 */
static bool password_display_callback(const uint8_t page, nbgl_pageContent_t *content) {
    UNUSED(page);
    content->type = INFOS_LIST;
    content->infosList.nbInfos = 1;
    content->infosList.infoTypes = &ptrToPwd[0];
    content->infosList.infoContents = &ptrToPwd[1];
    return true;
}

/**
 * @brief Display the passwords list
 *
 */
static void display_password(void) {
    password_list_reset();
    nbgl_useCaseNavigableContent("Your Password",
                                 0,
                                 1,
                                 display_choice_page,
                                 password_display_callback,
                                 NULL);
}

/**
 * @brief Show the password
 *
 * @param[in] index of the password
 *
 */
void show_password_cb(const size_t index) {
    ptrToPwd[0] = password_list_get_password(index);
    show_password_at_offset(password_list_get_offset(index), (uint8_t *) password_to_display);
    ptrToPwd[1] = &password_to_display[0];
    display_password();
}

/**
 * @brief Enter a password
 *
 * @param[in] index of the password
 *
 */
void type_password_cb(const size_t index) {
    type_password_at_offset(password_list_get_offset(index));
    display_success_page("PASSWORD HAS\nBEEN WRITTEN");
}

/**
 * @brief Create the password
 *
 */
static void create_password(void) {
    const size_t password_size = strlen(password_name);
    if (password_size == 0) {
        nbgl_useCaseStatus("The nickname\ncan't be empty", false, &display_create_pwd);
    } else {
        error_type_t error = create_new_password(password_name, password_size);
        if (error == OK) {
            display_success_page("NEW PASSWORD\nCREATED");
        } else {
            display_error_page(error);
        }
    }
}

/**
 * @brief Keyboard control callback
 *
 * @param[in] token button Id pressed
 * @param[in] index widget index on the page
 *
 */
static void keyboard_control_callback(const int token, const uint8_t index) {
    UNUSED(index);
    switch (token) {
        case BACK_BUTTON_TOKEN:
            release_context();
            display_choice_page();
            break;
        case CREATE_TOKEN:
            create_password();
            break;
        default:
            break;
    }
};

/**
 * @brief Keyboard press callback
 *
 * @param[in] touchedKey key pressed
 *
 */
static void key_press_callback(const char touchedKey) {
    uint32_t mask = 0;
    size_t textLen = strlen(password_name);
    if (touchedKey == BACKSPACE_KEY) {
        if (textLen == 0) {
            return;
        }
        password_name[--textLen] = '\0';
    } else {
        password_name[textLen] = touchedKey;
        password_name[++textLen] = '\0';
    }
    if (textLen >= MAX_METANAME) {
        // password name length can't be greater than MAX_METANAME, so we mask
        // every characters
        mask = -1;
    }
    nbgl_layoutUpdateKeyboardContent(layoutContext, &keyboardContent);
    nbgl_layoutUpdateKeyboard(layoutContext, keyboardIndex, mask, false, LOWER_CASE);
    nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_REFRESH, POST_REFRESH_FORCE_POWER_ON);
}

/**
 * @brief Display the passwords creation page
 *
 */
void display_create_pwd(void) {
    nbgl_layoutDescription_t layoutDescription = {
        .modal = false,
        .onActionCallback = &keyboard_control_callback
    };
    nbgl_layoutKbd_t kbdInfo = {
        .lettersOnly = false,
        .mode = MODE_LETTERS,
        .keyMask = 0,
        .casing = LOWER_CASE,
        .callback = &key_press_callback
    };
    nbgl_layoutHeader_t headerDesc = {
        .type = HEADER_BACK_AND_TEXT,
        .backAndText.token = BACK_BUTTON_TOKEN,
#ifdef HAVE_PIEZO_SOUND
        .backAndText.tuneId = TUNE_TAP_CASUAL,
#endif
    };
    confirmButton = (nbgl_layoutConfirmationButton_t){
        .text = "Create password",
        .token = CREATE_TOKEN,
        .active = true,
    };
    keyboardContent = (nbgl_layoutKeyboardContent_t){
        .type = KEYBOARD_WITH_BUTTON,
        .title = "New password nickname",
        .text = password_name,
        .textToken = KBD_TEXT_TOKEN,
        .confirmationButton = confirmButton,
#ifdef HAVE_PIEZO_SOUND
        .tuneId = TUNE_TAP_CASUAL,
#endif
    };

    password_name[0] = '\0';

    // Create page layout
    release_context();
    layoutContext = nbgl_layoutGet(&layoutDescription);

    // Add header
    nbgl_layoutAddHeader(layoutContext, &headerDesc);

    // Add keyboard
    keyboardIndex = nbgl_layoutAddKeyboard(layoutContext, &kbdInfo);
    if (keyboardIndex < 0) {
        // Error
        release_context();
        return;
    }

    nbgl_layoutAddKeyboardContent(layoutContext, &keyboardContent);
    nbgl_layoutDraw(layoutContext);
    nbgl_refresh();
}
