#include <os.h>
#include <string.h>

#include "ui_fatstacks.h"
#include "password_options.h"

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
    UPPERCASE_TOKEN,
    LOWERCASE_TOKEN,
    NUMBERS_TOKEN,
    BARS_TOKEN,
    EXT_SYMBOLS_TOKEN,
    CHOICE_WRITE_TOKEN,
    CHOICE_DISPLAY_TOKEN,
    CHOICE_CREATE_TOKEN,
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

static void display_keyboard_page(void);

/*
 * About menu
 */
static const char *const infoTypes[] = {"Version", "Passwords"};
static const char *const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};

static void display_choose_pwd_options_page(void);
static void display_name_pwd_page(void);

static bool onInfos(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 2;
        content->infosList.infoTypes = (const char **) infoTypes;
        content->infosList.infoContents = (const char **) infoContents;
    } else {
        return false;
    }
    return true;
}

/*
 * Charset options page & callback
 */
static const nbgl_layoutSwitch_t switches[] = {
    {
        .initState = true,
        .text = "Enable uppercase",
        .subText = NULL,
        .token = UPPERCASE_TOKEN,
        .tuneId = TUNE_TAP_CASUAL
    },
    {
        .initState = true,
        .text = "Enable lowecase",
        .subText = NULL,
        .token = LOWERCASE_TOKEN,
        .tuneId = TUNE_TAP_CASUAL
    },
    {
        .initState = true,
        .text = "Enable numbers",
        .subText = NULL,
        .token = NUMBERS_TOKEN,
        .tuneId = TUNE_TAP_CASUAL
    },
    {
        .initState = false,
        .text = "Enable separators",
        .subText = "('-', ' ', '_')",
        .token = BARS_TOKEN,
        .tuneId = TUNE_TAP_CASUAL
    },
    {
        .initState = false,
        .text = "Enable special characters",
        .subText = NULL,
        .token = EXT_SYMBOLS_TOKEN,
        .tuneId = TUNE_TAP_CASUAL
    }
};

static void charset_settings_callback(const int token) {
    PRINTF("Settings callback TOKEN %d triggered\n", token);
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
    }
    PRINTF("Changed the mapping, now is: 0x%0x\n", get_charset_options());
}

static void page_callback(const int token, const uint8_t index) {
    PRINTF("Create password callback: token %d - index %d\n", token, index);
    if (token == 0) {
        // navigation case
        switch (index) {
            case 0:
                // first page, no token: choosing the charset option
                display_choose_pwd_options_page();
                break;
            case 1:
                // second page: naming the password
                display_name_pwd_page();
                break;
            case (uint8_t)-1:
                // quit button: go back to home page
                display_home_page();
                break;
            default:
                break;
        }
    } else {

        charset_settings_callback(token);
    }
};


static void display_choose_pwd_options_page() {
    PRINTF("Choose pwd options\n");
    release_context();
    nbgl_pageContent_t content = {
        .title = "Password options",
        .isTouchableTitle = false,
        .tuneId = TUNE_TAP_CASUAL,
        .type = SWITCHES_LIST,
        .switchesList = {
            .switches = (nbgl_layoutSwitch_t*)switches,
            .nbSwitches = 5
        }
    };
    nbgl_pageNavigationInfo_t navigation = {
        .navType = NAV_WITH_BUTTONS,
        .activePage = 0,
        .nbPages = 2,
        .tuneId = TUNE_TAP_CASUAL,
        .navWithButtons = {
            .quitButton = true,
        },
    };
    nbgl_pageDrawGenericContent(&page_callback, &navigation, &content);
    nbgl_refresh();
}


static void display_name_pwd_page() {
    PRINTF("Name pwd\n");
    release_context();
    nbgl_pageContent_t content = {
        .title = "Password options",
        .isTouchableTitle = false,
        .tuneId = TUNE_TAP_CASUAL,
    };
    nbgl_pageNavigationInfo_t navigation = {
        .navType = NAV_WITH_BUTTONS,
        .activePage = 1,
        .nbPages = 2,
        .tuneId = TUNE_TAP_CASUAL,
        .navWithButtons = {
            .quitButton = true,
        },
    };
    nbgl_pageDrawGenericContent(&page_callback, &navigation, &content);
    nbgl_refresh();
}

/*
 * Keyboard page & callback
 */
static char password_name[120] = {0};
static int textIndex, keyboardIndex = 0;

static void keyboard_dispatcher(const int token, uint8_t index __attribute__((unused))) {
    PRINTF("keyboard dispatcher, with %d\n", token);
    if (token == BACK_BUTTON_TOKEN) {
        release_context();
        display_choose_pwd_options_page();
    }
}

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

// TODO: add static
void display_keyboard_page() {
    nbgl_layoutDescription_t layoutDescription = {.modal = false,
                                                  .onActionCallback = &keyboard_dispatcher};
    nbgl_layoutKbd_t kbdInfo = {.lettersOnly = false,   // use only letters
                                .upperCase = false,   // start with lower case letters
                                .mode = MODE_LETTERS,  // start in letters mode
                                .keyMask = 0,          // no inactive key
                                .callback = &key_press_callback};
    nbgl_layoutCenteredInfo_t centeredInfo = {.text1 = NULL,
                                              .text2 = "Enter a nickname for\nthis new password",
                                              .text3 = NULL,
                                              .style = LARGE_CASE_INFO,
                                              .icon = NULL,
                                              .offsetY = 0,
                                              .onTop = true};
    strlcpy(password_name, "", 1);
    layoutContext = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddProgressIndicator(layoutContext, 0, 0, true, BACK_BUTTON_TOKEN, TUNE_TAP_CASUAL);
    nbgl_layoutAddCenteredInfo(layoutContext, &centeredInfo);
    keyboardIndex = nbgl_layoutAddKeyboard(layoutContext, &kbdInfo);
    textIndex = nbgl_layoutAddEnteredText(layoutContext, false, 0, password_name, false, 32);
    nbgl_layoutAddFooter(layoutContext, "Use this name", CREATE_2_TOKEN, TUNE_TAP_CASUAL);
    nbgl_layoutDraw(layoutContext);
}


/*
 * Choice page (create, print, display) & dispatcher
 */

static const char* const bars[] = {
    "Type a password", "Show a password", "Create a new password"
};
static const uint8_t barsToken[] = {CHOICE_WRITE_TOKEN, CHOICE_DISPLAY_TOKEN, CHOICE_CREATE_TOKEN,};

static bool choice_navigation_callback(const uint8_t page __attribute__((unused)), nbgl_pageContent_t *content) {
    content->type = BARS_LIST;
    content->barsList.nbBars = 3;
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
            display_choose_pwd_options_page();
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
        nbgl_useCaseSettings("Passwords infos", 0, 1, false, display_home_page, onInfos, NULL);
    } else if (token == CHOOSE_ACTION_TOKEN) {
        display_choice_page();
    } else if (token == BACK_HOME_TOKEN) {
        display_home_page();
    }
}

void display_home_page(void) {
    init_charset_options();
    nbgl_pageInfoDescription_t home = {
        .centeredInfo.icon = NULL,
        .centeredInfo.text1 = "Passwords",
        .centeredInfo.text2 = "Create, manage and type\npasswords through\nyour device",
        .centeredInfo.text3 = NULL,
        .centeredInfo.style = LARGE_CASE_INFO,
        .centeredInfo.offsetY = 32,
        .topRightStyle = QUIT_ICON,
        .bottomButtonStyle = INFO_ICON,
        .topRightToken = QUIT_APP_TOKEN,
        .bottomButtonToken = INFO_TOKEN,
        .footerText = NULL,
        .tapActionText = "Tap to start the app",
        .tapActionToken = CHOOSE_ACTION_TOKEN,
        .tuneId = TUNE_TAP_CASUAL};
    release_context();
    pageContext = nbgl_pageDrawInfo(&home_dispatcher, NULL, &home);
    nbgl_refresh();
}

#endif
