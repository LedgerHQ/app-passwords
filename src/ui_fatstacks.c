#include <os.h>
#include <string.h>

#include "error.h"
#include "globals.h"
#include "metadata.h"
#include "options.h"
#include "password.h"
#include "ui_fatstacks.h"

#if defined(TARGET_FATSTACKS)

#include <nbgl_layout.h>
#include <nbgl_page.h>
#include <nbgl_use_case.h>

static nbgl_page_t *pageContext;
static nbgl_layout_t *layoutContext = 0;
#define MAX_ERROR_MSG_SIZE 100
static char errorMessage[MAX_ERROR_MSG_SIZE] = {0};

enum {
    BACK_BUTTON_TOKEN = FIRST_USER_TOKEN,
    BACK_HOME_TOKEN,
    CREATE_1_TOKEN,
    CREATE_2_TOKEN,
    CHOOSE_ACTION_TOKEN,
    INFO_TOKEN,
    QUIT_APP_TOKEN,
    CREATE_NAVIGATION_TOKEN,
    CREATE_TOKEN,
    UPPERCASE_TOKEN,
    LOWERCASE_TOKEN,
    NUMBERS_TOKEN,
    BARS_TOKEN,
    EXT_SYMBOLS_TOKEN,
    CHOICE_WRITE_TOKEN,
    CHOICE_DISPLAY_TOKEN,
    CHOICE_CREATE_TOKEN,
    CHOICE_DELETE_TOKEN,
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
 * About menu
 */
static const char *const infoTypes[] = {"Version", "Passwords"};
static const char *const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};

static nbgl_layoutSwitch_t switches[5];

static bool display_settings_navigation(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        switches[0] = (nbgl_layoutSwitch_t){.initState = has_charset_option(UPPERCASE_BITFLAG),
                                            .text = "Enable uppercase",
                                            .subText = NULL,
                                            .token = UPPERCASE_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[1] = (nbgl_layoutSwitch_t){.initState = has_charset_option(LOWERCASE_BITFLAG),
                                            .text = "Enable lowecase",
                                            .subText = NULL,
                                            .token = LOWERCASE_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[2] = (nbgl_layoutSwitch_t){.initState = has_charset_option(NUMBERS_BITFLAG),
                                            .text = "Enable numbers",
                                            .subText = NULL,
                                            .token = NUMBERS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[3] = (nbgl_layoutSwitch_t){.initState = has_charset_option(BARS_BITFLAG),
                                            .text = "Enable separators",
                                            .subText = "('-', ' ', '_')",
                                            .token = BARS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        switches[4] = (nbgl_layoutSwitch_t){.initState = has_charset_option(EXT_SYMBOLS_BITFLAG),
                                            .text = "Enable special characters",
                                            .subText = NULL,
                                            .token = EXT_SYMBOLS_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};

        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = 5;
        content->switchesList.switches = &switches[0];
    } else if (page == 1) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 2;
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
        default:
            break;
    }
    PRINTF("New settings value: %d\n", get_charset_options());
}

static void check_settings_before_home() {
    PRINTF("Current settings value: %d\n", get_charset_options());
    if (get_charset_options() == 0) {
        nbgl_useCaseStatus("At least one charset\nmust be selected", false, &display_settings_page);
    } else {
        display_home_page();
    }
}

static void display_settings_page() {
    nbgl_useCaseSettings("Passwords infos",
                         0,
                         2,
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
            PRINTF("The error nb %d occured\n", error);
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
    PRINTF("Current text is: '%s' (size '%d')\n", password_name, textLen);
    nbgl_layoutUpdateKeyboard(layoutContext, keyboardIndex, mask);
    nbgl_layoutUpdateEnteredText(layoutContext, textIndex, false, 0, &(password_name[0]), false);
    nbgl_refresh();
}

static void display_create_pwd_page() {
    release_context();
    nbgl_layoutDescription_t layoutDescription = {.modal = false,
                                                  .onActionCallback = &page_callback};
    nbgl_layoutKbd_t kbdInfo = {.lettersOnly = false,
                                .upperCase = false,
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
    textIndex = nbgl_layoutAddEnteredText(layoutContext, false, 0, password_name, false, 32);
    nbgl_layoutDraw(layoutContext);
}

/*
 * Password list
 * Used to go to either print, display or delete
 */

#define DISPLAYED_PASSWORD_PER_PAGE 5

/* Buffer where password name strings are stored */
static char pwdBuffer[DISPLAYED_PASSWORD_PER_PAGE * (MAX_METANAME + 1)] = {0};

/* Placeholder for currently displayed password names, points to pwdBuffer indexes */
static const char *passwordList[DISPLAYED_PASSWORD_PER_PAGE] = {0};

/* Index of the last displayed password */
static size_t pwdIndex = 0;

/* Display password offsets, to be able to retrieve them in metadatas */
static size_t pwdOffsets[DISPLAYED_PASSWORD_PER_PAGE] = {0};

/* Callback to trigger with password offset - print, display or delete */
void (*selector_callback)(const size_t offset);

static bool display_password_list_navigation(uint8_t page, nbgl_pageContent_t *content) {
    pwdIndex = page * DISPLAYED_PASSWORD_PER_PAGE;
    pwdBuffer[0] = '\0';
    size_t bufferOffset = 0;
    size_t localNb = 0;
    while (localNb < DISPLAYED_PASSWORD_PER_PAGE && pwdIndex < N_storage.metadata_count) {
        size_t pwdOffset = get_metadata(pwdIndex);
        if (pwdOffset == -1UL) {
            break;
        }
        pwdOffsets[localNb] = pwdOffset;
        const size_t pwdLength = METADATA_NICKNAME_LEN(pwdOffset);
        char *nextBufferOffset = &pwdBuffer[0] + bufferOffset;
        strlcpy(nextBufferOffset, (void *) METADATA_NICKNAME(pwdOffset), pwdLength + 1);
        bufferOffset += (pwdLength + 1);
        passwordList[localNb] = nextBufferOffset;

        localNb++;
        pwdIndex++;
    }

    content->type = CHOICES_LIST;
    content->choicesList.names = (char **) passwordList;
    content->choicesList.localized = false;
    content->choicesList.nbChoices = localNb;
    content->choicesList.initChoice = 0;
    content->choicesList.token = CHOOSE_ACTION_TOKEN;
    content->choicesList.tuneId = TUNE_TAP_CASUAL;
    return true;
}

static void password_list_callback(const int token __attribute__((unused)), const uint8_t index) {
    selector_callback(pwdOffsets[index]);
}

static void display_password_list_page() {
    pwdIndex = 0;
    pwdBuffer[0] = '\0';
    nbgl_useCaseSettings("Passwords infos",
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
static uint8_t password_to_display[PASSWORD_MAX_SIZE + 1] = {0};
static const uint8_t *ptrToPwd[1] = {0};

static bool display_password_navigation(uint8_t page __attribute__((unused)),
                                        nbgl_pageContent_t *content) {
    ptrToPwd[0] = &password_to_display[0];
    content->type = INFOS_LIST;
    content->infosList.nbInfos = 1;
    content->infosList.infoTypes = (const char **) ptrToPwd;
    content->infosList.infoContents = (const char **) ptrToPwd;
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

void type_password_cb(const size_t offset) {
    PRINTF("type\n");
    type_password_at_offset(offset);
    display_success_page("PASSWORD HAS\nBEEN WRITTEN");
}

void show_password_cb(const size_t offset) {
    PRINTF("show\n");
    show_password_at_offset(offset, password_to_display);
    display_password_page();
}

void delete_password_cb(const size_t offset) {
    PRINTF("delete\n");
    error_type_t result = delete_password_at_offset(offset);
    if (result == OK) {
        display_success_page("PASSWORD HAS\nBEEN DELETED");
    } else {
        display_error_page(result);
    }
}

/*
 * Delete confirmation page
 */

#define DELETION_MSG_SIZE 37
#define DELETION_BUFFER_MAX DELETION_MSG_SIZE - 2 + MAX_METANAME + 1
static const char confirmDeletion[DELETION_MSG_SIZE + 1] = \
    "Confirm the deletion\nof password\n'%s'";
static char deletionBuffer[DELETION_BUFFER_MAX] = {0};
static nbgl_layoutTagValueList_t pairList = {
  .nbMaxLinesForValue = 0,
  .nbPairs = 0,
  .pairs = NULL
};

static void reviewChoice(bool confirm) {
    PRINTF("OK, confirm: %d\n", confirm);
    if (confirm) {
        delete_password_cb(0);
    }
    else {
        display_choice_page();
    }
}

static void confirm_password_deletion(const size_t offset) {
    PRINTF("current index = %d\n", pwdIndex);
    PRINTF("Format = '%s'\n", confirmDeletion);
    PRINTF("Pwd: '%s'\n", passwordList[1]);
    snprintf(&deletionBuffer[0], DELETION_BUFFER_MAX, confirmDeletion, passwordList[1]);
    PRINTF("Result: '%s'\n", &deletionBuffer[0]);

    nbgl_pageInfoLongPress_t infoLongPress = {
        .icon = NULL,
        .text = &deletionBuffer[0],
        .longPressText = "Hold to confirm"
    };

    PRINTF("GO\n");
    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Don't remove the password", reviewChoice);
}

/*
 * Choice page (create, print, display, delete) & dispatcher
 */

static const char *const bars[] = {"Type a password",
                                   "Show a password",
                                   "Create a new password",
                                   "Delete a password"};
static const uint8_t barsToken[] = {CHOICE_WRITE_TOKEN,
                                    CHOICE_DISPLAY_TOKEN,
                                    CHOICE_CREATE_TOKEN,
                                    CHOICE_DELETE_TOKEN};

static bool choice_navigation_callback(const uint8_t page __attribute__((unused)),
                                       nbgl_pageContent_t *content) {
    content->type = BARS_LIST;
    content->barsList.nbBars = 4;
    content->barsList.barTexts = (const char **const) bars;
    content->barsList.tokens = (uint8_t *const) barsToken;
    return true;
}

static void choice_callback(const int token, const uint8_t index __attribute__((unused))) {
    switch (token) {
        case CHOICE_WRITE_TOKEN:
            PRINTF("Write password option\n");
            selector_callback = type_password_cb;
            display_password_list_page();
            break;
        case CHOICE_DISPLAY_TOKEN:
            PRINTF("Display password option\n");
            selector_callback = show_password_cb;
            display_password_list_page();
            break;
        case CHOICE_CREATE_TOKEN:
            PRINTF("Create new password option\n");
            display_create_pwd_page();
            break;
        case CHOICE_DELETE_TOKEN:
            PRINTF("Display password option\n");
            selector_callback = confirm_password_deletion;
            display_password_list_page();
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
 * Home page & dispatcher
 */
static void home_dispatcher(int token, uint8_t index __attribute__((unused))) {
    if (token == QUIT_APP_TOKEN) {
        release_context();
        os_sched_exit(-1);
    } else if (token == INFO_TOKEN) {
        release_context();
        display_settings_page();
    } else if (token == CHOOSE_ACTION_TOKEN) {
        display_choice_page();
    } else if (token == BACK_HOME_TOKEN) {
        display_home_page();
    }
}

void display_home_page(void) {
    nbgl_pageInfoDescription_t home = {
        .centeredInfo.icon = NULL,
        .centeredInfo.text1 = "Passwords",
        .centeredInfo.text2 = "Create, type and display\npasswords through\nyour device",
        .centeredInfo.text3 = NULL,
        .centeredInfo.style = LARGE_CASE_INFO,
        .centeredInfo.offsetY = 32,
        .topRightStyle = QUIT_ICON,
        .bottomButtonStyle = INFO_ICON,
        .topRightToken = QUIT_APP_TOKEN,
        .bottomButtonsToken = INFO_TOKEN,
        .footerText = NULL,
        .tapActionText = "Tap to manage\nyour passwords",
        .tapActionToken = CHOOSE_ACTION_TOKEN,
        .tuneId = TUNE_TAP_CASUAL};
    release_context();
    pageContext = nbgl_pageDrawInfo(&home_dispatcher, NULL, &home);
    nbgl_refresh();
}

#endif
