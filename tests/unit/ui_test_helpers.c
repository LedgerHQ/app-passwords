#include <string.h>

#include "unity.h"

#include "ui_test_helpers.h"
#include "hid_mapping.h"
#include "options.h"
#include "password.h"
#include "password_list.h"
#include "fake_vars.h"
#include "Mockdispatcher.h"
#include "Mockio.h"
#include "Mockmain_std_app.h"
#include "Mocknbgl_use_case.h"
#include "Mockpassword_typing.h"

screen_t screen;
size_t screen_count;
uint8_t choices_per_page;

bool generation_result;
size_t generation_calls;
const uint8_t *generation_out;

size_t app_exit_calls;
size_t dispatch_calls;
size_t sw_count;
uint16_t last_sw;

static void new_screen(screen_type_t type) {
    memset(&screen, 0, sizeof(screen));
    screen.type = type;
    screen_count++;
}

static void fake_status(const char *message,
                        bool isSuccess,
                        nbgl_callback_t quitCallback,
                        int cmock_num_calls) {
    (void) cmock_num_calls;
    new_screen(SCREEN_STATUS);
    screen.title = message;
    screen.success = isSuccess;
    screen.quit = quitCallback;
}

static void fake_static_review(const nbgl_contentTagValueList_t *tagValueList,
                               const nbgl_pageInfoLongPress_t *infoLongPress,
                               const char *rejectText,
                               nbgl_choiceCallback_t callback,
                               int cmock_num_calls) {
    (void) tagValueList;
    (void) cmock_num_calls;
    new_screen(SCREEN_STATIC_REVIEW);
    screen.long_press = *infoLongPress;
    screen.sub_message = rejectText;
    screen.choice = callback;
}

static void fake_choice(const nbgl_icon_details_t *icon,
                        const char *message,
                        const char *subMessage,
                        const char *confirmText,
                        const char *rejectString,
                        nbgl_choiceCallback_t callback,
                        int cmock_num_calls) {
    (void) confirmText;
    (void) rejectString;
    (void) cmock_num_calls;
    new_screen(SCREEN_CHOICE);
    screen.icon = icon;
    screen.title = message;
    screen.sub_message = subMessage;
    screen.choice = callback;
}

static void fake_generic_configuration(const char *title,
                                       uint8_t initPage,
                                       const nbgl_genericContents_t *contents,
                                       nbgl_callback_t quitCallback,
                                       int cmock_num_calls) {
    (void) cmock_num_calls;
    new_screen(SCREEN_GENERIC_CONFIGURATION);
    screen.title = title;
    screen.init_page = initPage;
    screen.contents = contents;
    screen.quit = quitCallback;
}

static void fake_keyboard(const nbgl_keyboardParams_t *params,
                          nbgl_callback_t backCallback,
                          int cmock_num_calls) {
    (void) cmock_num_calls;
    new_screen(SCREEN_KEYBOARD);
    screen.keyboard = *params;
    screen.title = params->title;
    screen.quit = backCallback;
}

static void fake_home_and_settings(const char *appName,
                                   const nbgl_icon_details_t *appIcon,
                                   const char *tagline,
                                   const uint8_t initSettingPage,
                                   const nbgl_genericContents_t *settingContents,
                                   const nbgl_contentInfoList_t *infosList,
                                   const nbgl_homeAction_t *action,
                                   nbgl_callback_t quitCallback,
                                   int cmock_num_calls) {
    (void) tagline;
    (void) infosList;
    (void) cmock_num_calls;
    new_screen(SCREEN_HOME_AND_SETTINGS);
    screen.title = appName;
    screen.icon = appIcon;
    screen.init_page = initSettingPage;
    screen.contents = settingContents;
    screen.home_action = action;
    screen.quit = quitCallback;
}

static void fake_navigable_content(const char *title,
                                   uint8_t initPage,
                                   uint8_t nbPages,
                                   nbgl_callback_t quitCallback,
                                   nbgl_navCallback_t navCallback,
                                   nbgl_layoutTouchCallback_t controlsCallback,
                                   int cmock_num_calls) {
    (void) nbPages;
    (void) cmock_num_calls;
    new_screen(SCREEN_NAVIGABLE_CONTENT);
    screen.title = title;
    screen.init_page = initPage;
    screen.quit = quitCallback;
    screen.nav = navCallback;
    screen.touch = controlsCallback;
}

static uint8_t fake_nb_choices_in_page(uint8_t nbChoices,
                                       const nbgl_contentRadioChoice_t *choicesList,
                                       uint8_t startIndex,
                                       bool withNav,
                                       int cmock_num_calls) {
    (void) nbChoices;
    (void) choicesList;
    (void) startIndex;
    (void) withNav;
    (void) cmock_num_calls;
    return choices_per_page;
}

static bool fake_type_password(uint8_t *data,
                               uint32_t dataSize,
                               uint8_t *out,
                               setmask_t setMask,
                               uint8_t version,
                               uint32_t size,
                               int cmock_num_calls) {
    (void) data;
    (void) dataSize;
    (void) setMask;
    (void) version;
    (void) size;
    (void) cmock_num_calls;
    generation_calls++;
    generation_out = out;
    if (generation_result && (out != NULL)) {
        strcpy((char *) out, GENERATED_PASSWORD);
    }
    return generation_result;
}

static void fake_app_exit(int cmock_num_calls) {
    (void) cmock_num_calls;
    app_exit_calls++;
}

static int fake_dispatch(int cmock_num_calls) {
    (void) cmock_num_calls;
    dispatch_calls++;
    return 0;
}

static int fake_io_send_response_buffers(const buffer_t *rdatalist,
                                         size_t count,
                                         uint16_t sw,
                                         int cmock_num_calls) {
    (void) rdatalist;
    (void) count;
    (void) cmock_num_calls;
    sw_count++;
    last_sw = sw;
    return 0;
}

void ui_test_setup(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    N_storage_rw.keyboard_layout = HID_MAPPING_QWERTY;
    init_charset_options();
    memset(&app_state, 0, sizeof(app_state));
    password_list_reset();

    memset(&screen, 0, sizeof(screen));
    screen_count = 0;
    choices_per_page = 4;
    generation_result = true;
    generation_calls = 0;
    generation_out = NULL;
    app_exit_calls = 0;
    dispatch_calls = 0;
    sw_count = 0;
    last_sw = 0;

    Mocknbgl_use_case_Init();
    Mockmain_std_app_Init();
    Mockio_Init();
    Mockdispatcher_Init();
    Mockpassword_typing_Init();
    nbgl_useCaseStatus_StubWithCallback(fake_status);
    nbgl_useCaseStaticReview_StubWithCallback(fake_static_review);
    nbgl_useCaseChoice_StubWithCallback(fake_choice);
    nbgl_useCaseGenericConfiguration_StubWithCallback(fake_generic_configuration);
    nbgl_useCaseKeyboard_StubWithCallback(fake_keyboard);
    nbgl_useCaseHomeAndSettings_StubWithCallback(fake_home_and_settings);
    nbgl_useCaseNavigableContent_StubWithCallback(fake_navigable_content);
    nbgl_useCaseGetNbChoicesInPage_StubWithCallback(fake_nb_choices_in_page);
    type_password_StubWithCallback(fake_type_password);
    app_exit_StubWithCallback(fake_app_exit);
    dispatch_StubWithCallback(fake_dispatch);
    io_send_response_buffers_StubWithCallback(fake_io_send_response_buffers);
}

void ui_test_teardown(void) {
    Mocknbgl_use_case_Verify();
    Mockmain_std_app_Verify();
    Mockio_Verify();
    Mockdispatcher_Verify();
    Mockpassword_typing_Verify();
    Mocknbgl_use_case_Destroy();
    Mockmain_std_app_Destroy();
    Mockio_Destroy();
    Mockdispatcher_Destroy();
    Mockpassword_typing_Destroy();
}

void add_password(const char *nickname) {
    TEST_ASSERT_EQUAL(OK, create_new_password(nickname, strlen(nickname)));
}
