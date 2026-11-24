
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

// Flag to indicate if all passwords are being deleted
static bool all_passwords;

// Keyboard contexts
// Holds either a nickname being typed or one read back from storage, plus its NUL terminator.
static char password_name[MAX_METANAME + 1] = {0};
_Static_assert(MAX_NICKNAME_LEN < sizeof(password_name),
               "NBGL writes the terminator at entryBuffer[entryMaxLen], so display_create_pwd()'s "
               "entryMaxLen must stay inside password_name");

static nbgl_genericContents_t genericContent = {0};
static nbgl_content_t contentsList = {0};
// Number of entries actually published to NBGL by the last display_password_list().
static size_t nbDisplayedPasswords = 0;
// Entries per page of the passwords list (touch screens only).
static size_t nbPasswordsPerPage = 0;

// A list longer than the threshold is split into slices, each opening its own list.
#define PASSWORDS_SLICE_THRESHOLD 20
#define PASSWORDS_SLICE_SIZE      15
#define MAX_SLICES                ((MAX_METADATA_COUNT + PASSWORDS_SLICE_SIZE - 1) / PASSWORDS_SLICE_SIZE)
// "<first> - <last>"
#define SLICE_LABEL_SIZE (2 * MAX_NICKNAME_LEN + 4)
static char sliceLabels[MAX_SLICES][SLICE_LABEL_SIZE] = {0};
static const char *sliceNames[MAX_SLICES] = {0};
// One token per slice, from FIRST_SLICE_TOKEN
#define FIRST_SLICE_TOKEN FIRST_USER_TOKEN
static uint8_t sliceTokens[MAX_SLICES] = {0};
_Static_assert(FIRST_SLICE_TOKEN + MAX_SLICES <= UINT8_MAX, "slice tokens must fit a byte");
static size_t nbSlices = 0;
// Slice last opened, shown again when leaving its list
static size_t currentSlice = 0;
// Entries of the list currently shown: [sliceStart, sliceStart + sliceCount)
static size_t sliceStart = 0;
static size_t sliceCount = 0;

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
#ifdef SCREEN_SIZE_WALLET
    nbgl_contentInfoLongPress_t infoLongPress = {.icon = NULL,
                                                 .text = msgBuffer,
                                                 .longPressText = "Hold to confirm"};
#endif

    if (index == -1UL) {
        // All passwords
        all_passwords = true;
        snprintf(msgBuffer,
                 sizeof(msgBuffer),
                 "Confirm the deletion\nof all passwords (%u)",
                 (unsigned int) N_storage.metadata_count);
    } else {
        // A single password
        all_passwords = false;
        snprintf(msgBuffer,
                 sizeof(msgBuffer),
                 "Confirm the deletion\nof password\n'%s'",
                 password_list_get_password(index));
        password_list_set_current(index);
    }
#ifdef SCREEN_SIZE_WALLET
    nbgl_useCaseStaticReview(NULL, &infoLongPress, NULL, reviewDeletePassword_callback);
#else
    nbgl_useCaseChoice(NULL, msgBuffer, NULL, "Confirm", "Reject", reviewDeletePassword_callback);
#endif
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
    if (N_storage.metadata_count == 0) {
        nbgl_useCaseStatus("No passwords available", false, display_choice_page);
        return;
    }
    confirm_password_deletion(-1);
}

/**
 * @brief Absolute index of a choice selected in a CHOICES_LIST
 *
 * @param[in] index choice index forwarded by NBGL
 * @param[in] page page of the list
 * @param[in] perPage choices per page (touch screens only)
 * @return the index of the choice in the whole list
 *
 */
static size_t choice_absolute_index(const uint8_t index, const int page, const size_t perPage) {
#ifdef SCREEN_SIZE_WALLET
    // Touch screens paginate the list and forward the index within the page.
    return (size_t) page * perPage + index;
#else
    // Nano shows one choice per page and forwards the absolute index.
    UNUSED(page);
    UNUSED(perPage);
    return index;
#endif
}

/**
 * @brief Passwords control callback
 *
 * @param[in] token button Id pressed
 * @param[in] index widget index on the page
 * @param[in] page page of the list
 *
 */
static void password_callback(const int token, const uint8_t index, int page) {
    UNUSED(token);
    if (!selector_callback) {
        return;
    }
    const size_t slice_index = choice_absolute_index(index, page, nbPasswordsPerPage);
    // Single chokepoint for the show, type and delete callbacks: an index outside what was
    // actually published must never reach a metadata offset lookup.
    if (slice_index >= sliceCount || sliceStart + slice_index >= nbDisplayedPasswords) {
        return;
    }
    selector_callback(sliceStart + slice_index);
}

/**
 * @brief Display the passwords of the current slice
 *
 * @param[in] title list title
 * @param[in] quit_callback callback of the back action
 *
 */
static void display_slice_passwords(const char *title, nbgl_callback_t quit_callback) {
    explicit_bzero(&genericContent, sizeof(genericContent));
    explicit_bzero(&contentsList, sizeof(contentsList));
    genericContent.nbContents = 1;
    genericContent.contentsList = &contentsList;
    contentsList.type = CHOICES_LIST;
    contentsList.content.choicesList.nbChoices = sliceCount;
    contentsList.content.choicesList.names = &password_list_passwords()[sliceStart];
    contentsList.content.choicesList.token = -1;
#ifdef HAVE_PIEZO_SOUND
    contentsList.content.choicesList.tuneId = TUNE_TAP_CASUAL;
#endif
    contentsList.contentActionCallback = &password_callback;
#ifdef SCREEN_SIZE_WALLET
    nbPasswordsPerPage =
        nbgl_useCaseGetNbChoicesInPage(sliceCount, &contentsList.content.choicesList, 0, false);
#endif
    nbgl_useCaseGenericConfiguration(title, 0, &genericContent, quit_callback);
}

static void display_slices_list(void);

/**
 * @brief Slices control callback
 *
 * @param[in] token token of the slice pressed
 * @param[in] index widget index on the page
 * @param[in] page page of the list
 *
 */
static void slice_callback(const int token, const uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);
    if (token < FIRST_SLICE_TOKEN || (size_t) (token - FIRST_SLICE_TOKEN) >= nbSlices) {
        return;
    }
    const size_t slice = (size_t) (token - FIRST_SLICE_TOKEN);
    currentSlice = slice;
    sliceStart = slice * PASSWORDS_SLICE_SIZE;
    sliceCount = MIN(PASSWORDS_SLICE_SIZE, nbDisplayedPasswords - sliceStart);
    display_slice_passwords(sliceNames[slice], display_slices_list);
}

/**
 * @brief Display the list of slices
 *
 */
static void display_slices_list(void) {
    explicit_bzero(&genericContent, sizeof(genericContent));
    explicit_bzero(&contentsList, sizeof(contentsList));
    genericContent.nbContents = 1;
    genericContent.contentsList = &contentsList;
    contentsList.type = BARS_LIST;
    contentsList.content.barsList.nbBars = nbSlices;
    contentsList.content.barsList.barTexts = sliceNames;
    contentsList.content.barsList.tokens = sliceTokens;
#ifdef HAVE_PIEZO_SOUND
    contentsList.content.barsList.tuneId = TUNE_TAP_CASUAL;
#endif
    contentsList.contentActionCallback = &slice_callback;
#ifdef SCREEN_SIZE_WALLET
    const uint8_t nbSlicesPerPage =
        nbgl_useCaseGetNbBarsInPage(nbSlices, &contentsList.content.barsList, 0, false);
    // Touch screens start on the page holding the current slice, Nano on the slice itself.
    const uint8_t initPage = (nbSlicesPerPage > 0) ? currentSlice / nbSlicesPerPage : 0;
#else
    const uint8_t initPage = currentSlice;
#endif
    nbgl_useCaseGenericConfiguration("Passwords list",
                                     initPage,
                                     &genericContent,
                                     display_choice_page);
}

/**
 * @brief Length of the prefix of the entry `index` that tells it from `other1` and `other2`
 *
 */
static size_t distinctive_prefix_len(const size_t index, const size_t other1, const size_t other2) {
    size_t len = 0;
    if (other1 != index) {
        len = MAX(len, password_list_common_prefix_len(index, other1));
    }
    if (other2 != index) {
        len = MAX(len, password_list_common_prefix_len(index, other2));
    }
    return MIN(len + 1, strlen(password_list_get_password(index)));
}

/**
 * @brief Build the slices of the list
 *
 * Sorted list: each slice is labelled with its first and last nicknames, in upper case as the
 * order ignores the case. Otherwise: with the positions of its first and last entries.
 *
 */
static void build_slices(void) {
    nbSlices = (nbDisplayedPasswords + PASSWORDS_SLICE_SIZE - 1) / PASSWORDS_SLICE_SIZE;
    for (size_t slice = 0; slice < nbSlices; slice++) {
        const size_t first = slice * PASSWORDS_SLICE_SIZE;
        const size_t last = MIN(first + PASSWORDS_SLICE_SIZE, nbDisplayedPasswords) - 1;
        // Each end is shortened to what tells it from the neighbouring slice and the other end.
        const size_t previous = (first > 0) ? first - 1 : first;
        const size_t next = (last + 1 < nbDisplayedPasswords) ? last + 1 : last;
        if (!passwords_sorted_alphabetically()) {
            // Creation order: the nicknames are no range, label the slice by position.
            if (first == last) {
                snprintf(sliceLabels[slice], SLICE_LABEL_SIZE, "%u", (unsigned int) (first + 1));
            } else {
                snprintf(sliceLabels[slice],
                         SLICE_LABEL_SIZE,
                         "%u - %u",
                         (unsigned int) (first + 1),
                         (unsigned int) (last + 1));
            }
        } else if (first == last) {
            strlcpy(sliceLabels[slice], password_list_get_password(first), SLICE_LABEL_SIZE);
        } else {
            snprintf(sliceLabels[slice],
                     SLICE_LABEL_SIZE,
                     "%.*s - %.*s",
                     (int) distinctive_prefix_len(first, previous, last),
                     password_list_get_password(first),
                     (int) distinctive_prefix_len(last, next, first),
                     password_list_get_password(last));
        }
        for (char *c = sliceLabels[slice]; *c != '\0'; c++) {
            if (*c >= 'a' && *c <= 'z') {
                *c = (char) (*c - 'a' + 'A');
            }
        }
        sliceNames[slice] = sliceLabels[slice];
        sliceTokens[slice] = (uint8_t) (FIRST_SLICE_TOKEN + slice);
    }
}

/**
 * @brief Display the passwords list, split into slices when it is long
 *
 */
void display_password_list(void) {
    uint32_t pwdOffset = 0;
    size_t nbPasswords = 0;

    password_list_reset();
    nbDisplayedPasswords = 0;
    currentSlice = 0;
    sliceStart = 0;
    sliceCount = 0;
    while (nbPasswords < N_storage.metadata_count) {
        pwdOffset = get_metadata(nbPasswords);
        if (pwdOffset == UINT32_MAX) {
            break;
        }
        const size_t pwdLength = METADATA_NICKNAME_LEN(pwdOffset) + 1;
        // Stop at the capacity of the list arrays instead of carrying on: nbChoices below must
        // never exceed the number of entries actually stored in passwordList.
        if (!password_list_add_password(nbPasswords,
                                        pwdOffset,
                                        (void *) METADATA_NICKNAME(pwdOffset),
                                        pwdLength)) {
            break;
        }
        nbPasswords++;
    }
    if (nbPasswords == 0) {
        nbgl_useCaseStatus("No passwords available", false, display_choice_page);
        return;
    }
    if (passwords_sorted_alphabetically()) {
        password_list_sort(nbPasswords);
    }
    nbDisplayedPasswords = nbPasswords;
    if (nbPasswords > PASSWORDS_SLICE_THRESHOLD) {
        build_slices();
        display_slices_list();
        return;
    }
    sliceCount = nbPasswords;
    display_slice_passwords("Passwords list", display_choice_page);
}

/**
 * @brief Show the password
 *
 * @param[in] index of the password
 *
 */
/**
 * @brief Wipe the plaintext password left in the display buffer
 *
 */
void clear_displayed_password(void) {
    explicit_bzero(password_to_display, sizeof(password_to_display));
    ptrToPwd[1] = NULL;
}

/**
 * @brief Leave the password display page, wiping the plaintext first
 *
 */
static void display_choice_page_from_password(void) {
    clear_displayed_password();
    display_choice_page();
}

void show_password_cb(const size_t index) {
    const char *nickname = password_list_get_password(index);
    if (nickname == NULL) {
        display_choice_page();
        return;
    }
    clear_displayed_password();
    strlcpy(password_name, nickname, sizeof(password_name));
    ptrToPwd[0] = password_name;
    // On failure the buffer holds nothing, so say so rather than present an empty password.
    if (!show_password_at_offset(password_list_get_offset(index),
                                 (uint8_t *) password_to_display)) {
        clear_displayed_password();
        password_list_reset();
        nbgl_useCaseStatus("COULD NOT GENERATE\nTHE PASSWORD", false, display_choice_page);
        return;
    }
    ptrToPwd[1] = &password_to_display[0];

    password_list_reset();
    explicit_bzero(&genericContent, sizeof(genericContent));
    explicit_bzero(&contentsList, sizeof(contentsList));
    genericContent.nbContents = 1;
    genericContent.contentsList = &contentsList;
    contentsList.type = INFOS_LIST;
    contentsList.content.infosList.nbInfos = 1;
    contentsList.content.infosList.infoTypes = &ptrToPwd[0];
    contentsList.content.infosList.infoContents = &ptrToPwd[1];

    nbgl_useCaseGenericConfiguration("Your Password",
                                     0,
                                     &genericContent,
                                     display_choice_page_from_password);
}

/**
 * @brief Enter a password
 *
 * @param[in] index of the password
 *
 */
void type_password_cb(const size_t index) {
    // Nothing was typed if generation failed, so do not claim it was.
    if (!type_password_at_offset(password_list_get_offset(index))) {
        nbgl_useCaseStatus("COULD NOT GENERATE\nTHE PASSWORD", false, display_choice_page);
        return;
    }
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
    } else if (nickname_exists(password_name, password_size)) {
        nbgl_useCaseStatus("This nickname\nalready exists", false, &display_create_pwd);
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
 * @brief Display the passwords creation page
 *
 */
void display_create_pwd(void) {
    nbgl_kbdButtonParams_t confirmParams = {
#ifdef SCREEN_SIZE_WALLET
        .buttonText = "Create password",
#endif
        .onButtonCallback = &create_password,
    };

    nbgl_keyboardParams_t keyboardParams = {
        .type = KEYBOARD_WITH_BUTTON,
#ifdef SCREEN_SIZE_WALLET
        .title = "New password nickname",
#else
        .title = "Create password",
#endif
        .entryBuffer = password_name,
        // Only MAX_NICKNAME_LEN bytes of nickname fit in a metadata record, so accepting more
        // would show the user characters the device cannot keep. NBGL appends the typed
        // character and *then* the terminator, writing entryBuffer[entryMaxLen] on the last
        // accepted key; the assertion above keeps that write inside `password_name`.
        .entryMaxLen = MAX_NICKNAME_LEN,
        .lettersOnly = false,
#ifdef SCREEN_SIZE_WALLET
        .mode = MODE_LETTERS,
        .casing = LOWER_CASE,
#else
        .mode = MODE_NONE,
#endif
        .confirmationParams = confirmParams,
    };
    password_name[0] = '\0';
    nbgl_useCaseKeyboard(&keyboardParams, &display_choice_page);
}
