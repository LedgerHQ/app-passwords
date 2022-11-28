#include <os.h>
#include <string.h>

#include "ui_fatstacks.h"
#include "options.h"

#if defined(TARGET_FATSTACKS)

#include <nbgl_page.h>
#include <nbgl_use_case.h>

static nbgl_page_t *pageContext;
static nbgl_layout_t *layoutContext = 0;

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
    CHOICE_RESET_TOKEN,
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
static void display_name_pwd_page(void);
static void display_settings_page(void);

/*
 * About menu
 */
static const char *const infoTypes[] = {"Version", "Passwords"};
static const char *const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};

static nbgl_layoutSwitch_t switches[5];


static bool display_settings_navigation(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        switches[0] = (nbgl_layoutSwitch_t) {
            .initState = has_charset_option(UPPERCASE_BITFLAG),
            .text = "Enable uppercase",
            .subText = NULL,
            .token = UPPERCASE_TOKEN,
            .tuneId = TUNE_TAP_CASUAL
        };
        switches[1] = (nbgl_layoutSwitch_t) {
            .initState = has_charset_option(LOWERCASE_BITFLAG),
            .text = "Enable lowecase",
            .subText = NULL,
            .token = LOWERCASE_TOKEN,
            .tuneId = TUNE_TAP_CASUAL
        };
        switches[2] = (nbgl_layoutSwitch_t) {
            .initState = has_charset_option(NUMBERS_BITFLAG),
            .text = "Enable numbers",
            .subText = NULL,
            .token = NUMBERS_TOKEN,
            .tuneId = TUNE_TAP_CASUAL
        };
        switches[3] = (nbgl_layoutSwitch_t) {
            .initState = has_charset_option(BARS_BITFLAG),
            .text = "Enable separators",
            .subText = "('-', ' ', '_')",
            .token = BARS_TOKEN,
            .tuneId = TUNE_TAP_CASUAL
        };
        switches[4] = (nbgl_layoutSwitch_t) {
            .initState = has_charset_option(EXT_SYMBOLS_BITFLAG),
            .text = "Enable special characters",
            .subText = NULL,
            .token = EXT_SYMBOLS_TOKEN,
            .tuneId = TUNE_TAP_CASUAL
        };

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

static void charset_settings_callback(const int token, const uint8_t index __attribute__((unused))) {
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
    nbgl_useCaseSettings("Passwords infos", 0, 2, false, check_settings_before_home, display_settings_navigation, charset_settings_callback);
}

/*
 * Password creation & callback
 */
static char password_name[120] = {0};
static int textIndex, keyboardIndex = 0;

static void create_password(void) {
    PRINTF("Creating new password '%s'\n", password_name);
    release_context();
    if (strlen(password_name) == 0) {
        nbgl_useCaseStatus("The nickname\ncan't be empty", false, &display_name_pwd_page);
    } else {
        nbgl_useCaseStatus("NEW PASSWORD\nCREATED", true, &display_home_page);
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
    PRINTF("Current text is: '%s' (size '%d')\n", password_name, textLen);
    nbgl_layoutUpdateKeyboard(layoutContext, keyboardIndex, mask);
    nbgl_layoutUpdateEnteredText(layoutContext, textIndex, false, 0, &(password_name[0]), false);
    nbgl_refresh();
}

static void display_name_pwd_page() {
    release_context();
    nbgl_layoutDescription_t layoutDescription = {.modal = false, .onActionCallback = &page_callback};
    nbgl_layoutKbd_t kbdInfo = {.lettersOnly = false,
                                .upperCase = false,
                                .mode = MODE_LETTERS,
                                .keyMask = 0,
                                .callback = &key_press_callback};
    nbgl_layoutCenteredInfo_t centeredInfo = {
        .text1 = NULL,
        .text2 = "New password nickname",
        .text3 = NULL,
        .style = LARGE_CASE_INFO,
        .icon = NULL,
        .offsetY = 0,
        .onTop = true
    };
    layoutContext = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddProgressIndicator(layoutContext, 0, 0, true, BACK_BUTTON_TOKEN, TUNE_TAP_CASUAL);
    nbgl_layoutAddCenteredInfo(layoutContext, &centeredInfo);
    keyboardIndex = nbgl_layoutAddKeyboard(layoutContext, &kbdInfo);
    nbgl_layoutAddConfirmationButton(layoutContext, true, "Create password", CREATE_TOKEN, TUNE_TAP_CASUAL);
    strlcpy(password_name, "", 1);
    textIndex = nbgl_layoutAddEnteredText(layoutContext, false, 0, password_name, false, 32);
    nbgl_layoutDraw(layoutContext);
}

/*
 * Choice page (create, print, display) & dispatcher
 */

static const char* const bars[] = {
    "Type a password", "Show a password", "Create a new password", "Reset a password"
};
static const uint8_t barsToken[] = {CHOICE_WRITE_TOKEN, CHOICE_DISPLAY_TOKEN, CHOICE_CREATE_TOKEN, CHOICE_RESET_TOKEN};

static bool choice_navigation_callback(const uint8_t page __attribute__((unused)), nbgl_pageContent_t *content) {
    content->type = BARS_LIST;
    content->barsList.nbBars = 4;
    content->barsList.barTexts = (const char** const)bars;
    content->barsList.tokens = (uint8_t* const)barsToken;
    return true;
}

static void choice_callback(const int token, const uint8_t index __attribute__((unused))) {
    switch (token) {
        case CHOICE_WRITE_TOKEN:
            PRINTF("Write password option\n");
            break;
        case CHOICE_DISPLAY_TOKEN:
            PRINTF("Display password option\n");
            break;
        case CHOICE_CREATE_TOKEN:
            PRINTF("Create new password option\n");
            display_name_pwd_page();
            break;
    }
}

static void display_choice_page() {
    release_context();
    nbgl_useCaseSettings("What do you want to do?",
                         0,     // first page index
                         1,     // page number
                         false, // touchable title
                         display_home_page,
                         choice_navigation_callback,
                         choice_callback
        );
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
        .bottomButtonToken = INFO_TOKEN,
        .footerText = NULL,
        .tapActionText = "Tap to manage\nyour passwords",
        .tapActionToken = CHOOSE_ACTION_TOKEN,
        .tuneId = TUNE_TAP_CASUAL};
    release_context();
    pageContext = nbgl_pageDrawInfo(&home_dispatcher, NULL, &home);
    nbgl_refresh();
}

#endif
