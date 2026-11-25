#include <string.h>

#include "unity.h"

#include "ui.h"
#include "ui_test_helpers.h"
#include "globals.h"
#include "metadata.h"
#include "options.h"
#include "password.h"
#include "password_list.h"
#include "fake_vars.h"

// Indexes passed to the action selected in the passwords list
static size_t selected[8];
static size_t selected_count;

static void spy_selector(const size_t index) {
    TEST_ASSERT_TRUE(selected_count < sizeof(selected) / sizeof(selected[0]));
    selected[selected_count++] = index;
}

static const nbgl_content_t *list_content(void) {
    TEST_ASSERT_EQUAL(SCREEN_GENERIC_CONFIGURATION, screen.type);
    TEST_ASSERT_EQUAL(1, screen.contents->nbContents);
    return &screen.contents->contentsList[0];
}

static void add_three_passwords(void) {
    add_password("alpha");
    add_password("bravo");
    add_password("charlie");
}

static void check_status(const char *message, bool success) {
    TEST_ASSERT_EQUAL(SCREEN_STATUS, screen.type);
    TEST_ASSERT_EQUAL_STRING(message, screen.title);
    TEST_ASSERT_EQUAL(success, screen.success);
}

void setUp(void) {
    ui_test_setup();
    selector_callback = NULL;
    selected_count = 0;
}

void tearDown(void) {
    ui_test_teardown();
}

/* ----------------------------------------------------------------------- */
/* Passwords list                                                          */
/* ----------------------------------------------------------------------- */

static void test_password_list_empty(void) {
    display_password_list();
    check_status("No passwords available", false);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
}

static void test_password_list_shows_every_nickname(void) {
    add_three_passwords();

    display_password_list();
    TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
    const nbgl_content_t *content = list_content();
    TEST_ASSERT_EQUAL(CHOICES_LIST, content->type);
    TEST_ASSERT_EQUAL(3, content->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING("alpha", content->content.choicesList.names[0]);
    TEST_ASSERT_EQUAL_STRING("bravo", content->content.choicesList.names[1]);
    TEST_ASSERT_EQUAL_STRING("charlie", content->content.choicesList.names[2]);
}

static void test_password_list_sorted(void) {
    add_password("charlie");
    add_password("Bravo");
    add_password("alpha");
    N_storage_rw.passwords_order = PASSWORDS_ORDER_ALPHABETICAL;
    selector_callback = spy_selector;

    display_password_list();
    const nbgl_content_t *content = list_content();
    TEST_ASSERT_EQUAL_STRING("alpha", content->content.choicesList.names[0]);
    TEST_ASSERT_EQUAL_STRING("Bravo", content->content.choicesList.names[1]);
    TEST_ASSERT_EQUAL_STRING("charlie", content->content.choicesList.names[2]);

    // The selected entry keeps its own record
    content->contentActionCallback(0, 0, 0);
    TEST_ASSERT_EQUAL(1, selected_count);
    TEST_ASSERT_EQUAL(get_metadata(2), password_list_get_offset(selected[0]));
}

static void add_numbered_passwords(size_t count) {
    char name[8];
    for (size_t i = 0; i < count; i++) {
        snprintf(name, sizeof(name), "n%02u", (unsigned int) i);
        add_password(name);
    }
}

static void test_password_list_up_to_the_threshold(void) {
    add_numbered_passwords(20);

    display_password_list();
    TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
    TEST_ASSERT_EQUAL(20, list_content()->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING("n19", list_content()->content.choicesList.names[19]);
}

// Press the slice `slice` of the slices list
static void choose_slice(size_t slice) {
    const nbgl_contentBarsList_t *bars = &list_content()->content.barsList;
    TEST_ASSERT_TRUE(slice < bars->nbBars);
    list_content()->contentActionCallback(bars->tokens[slice], 0, 0);
}

static void test_password_list_slices(void) {
    add_numbered_passwords(21);
    choices_per_page = 4;
    selector_callback = spy_selector;

    display_password_list();
    TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
    TEST_ASSERT_EQUAL(0, screen.init_page);
    const nbgl_content_t *content = list_content();
    TEST_ASSERT_EQUAL(BARS_LIST, content->type);
    TEST_ASSERT_EQUAL(2, content->content.barsList.nbBars);
    // Creation order: labelled by position
    TEST_ASSERT_EQUAL_STRING("1 - 15", content->content.barsList.barTexts[0]);
    TEST_ASSERT_EQUAL_STRING("16 - 21", content->content.barsList.barTexts[1]);

    choose_slice(1);
    TEST_ASSERT_EQUAL_STRING("16 - 21", screen.title);
    content = list_content();
    TEST_ASSERT_EQUAL(CHOICES_LIST, content->type);
    TEST_ASSERT_EQUAL(6, content->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING("n15", content->content.choicesList.names[0]);
    TEST_ASSERT_EQUAL_STRING("n20", content->content.choicesList.names[5]);

    // Second entry of the second page: the index is absolute in the whole list
    content->contentActionCallback(0, 1, 1);
    // Out of the slice
    content->contentActionCallback(0, 2, 1);
    TEST_ASSERT_EQUAL(1, selected_count);
    TEST_ASSERT_EQUAL(20, selected[0]);
    TEST_ASSERT_EQUAL_STRING("n20", password_list_get_password(selected[0]));

    // Back to the slices
    screen.quit();
    TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
    TEST_ASSERT_EQUAL(2, list_content()->content.barsList.nbBars);
    TEST_ASSERT_EQUAL(0, screen.init_page);
    // Unknown tokens are ignored
    list_content()->contentActionCallback(FIRST_USER_TOKEN + 2, 0, 0);
    list_content()->contentActionCallback(FIRST_USER_TOKEN - 1, 0, 0);
    TEST_ASSERT_EQUAL_STRING("Passwords list", screen.title);
}

static void test_password_list_back_to_a_slice_on_another_page(void) {
    add_numbered_passwords(31);
    bars_per_page = 2;

    display_password_list();
    TEST_ASSERT_EQUAL(3, list_content()->content.barsList.nbBars);
    choose_slice(2);
    TEST_ASSERT_EQUAL(1, list_content()->content.choicesList.nbChoices);
    TEST_ASSERT_EQUAL_STRING("n30", list_content()->content.choicesList.names[0]);

    // Back on the second page, which holds the third slice
    screen.quit();
    TEST_ASSERT_EQUAL(BARS_LIST, list_content()->type);
    TEST_ASSERT_EQUAL(1, screen.init_page);

    // A new list starts on the first page
    display_password_list();
    TEST_ASSERT_EQUAL(0, screen.init_page);
}

static void test_password_list_slice_labels_by_position(void) {
    add_numbered_passwords(31);

    display_password_list();
    const nbgl_contentBarsList_t *bars = &list_content()->content.barsList;
    TEST_ASSERT_EQUAL(3, bars->nbBars);
    TEST_ASSERT_EQUAL_STRING("1 - 15", bars->barTexts[0]);
    TEST_ASSERT_EQUAL_STRING("16 - 30", bars->barTexts[1]);
    // A single entry keeps its position only
    TEST_ASSERT_EQUAL_STRING("31", bars->barTexts[2]);
}

static void test_password_list_slice_labels(void) {
    // Sorted, case ignored: "Bank1" to "Bank9" fill the first slice, "bankx00" and up the second
    char name[12];
    for (size_t i = 1; i <= 15; i++) {
        snprintf(name, sizeof(name), "%s%u", (i % 2) ? "Bank" : "bank", (unsigned int) i);
        add_password(name);
    }
    for (size_t i = 0; i < 15; i++) {
        snprintf(name, sizeof(name), "bankx%02u", (unsigned int) i);
        add_password(name);
    }
    add_password("zebra");
    N_storage_rw.passwords_order = PASSWORDS_ORDER_ALPHABETICAL;

    display_password_list();
    const nbgl_contentBarsList_t *bars = &list_content()->content.barsList;
    TEST_ASSERT_EQUAL(3, bars->nbBars);
    // Upper case, as the order ignores the case
    TEST_ASSERT_EQUAL_STRING("BANK1 - BANK9", bars->barTexts[0]);
    TEST_ASSERT_EQUAL_STRING("BANKX0 - BANKX1", bars->barTexts[1]);
    // A single entry keeps its whole nickname
    TEST_ASSERT_EQUAL_STRING("ZEBRA", bars->barTexts[2]);
}

// The list stops at the first record that cannot be read
static void test_password_list_stops_at_a_corrupted_record(void) {
    add_three_passwords();
    N_storage_rw.metadata_count = 4;

    display_password_list();
    TEST_ASSERT_EQUAL(3, list_content()->content.choicesList.nbChoices);
}

static void test_password_list_selection_is_absolute(void) {
    add_three_passwords();
    choices_per_page = 2;
    selector_callback = spy_selector;
    display_password_list();
    const nbgl_content_t *content = list_content();

    content->contentActionCallback(0, 1, 0);
    content->contentActionCallback(0, 0, 1);
    TEST_ASSERT_EQUAL(2, selected_count);
    TEST_ASSERT_EQUAL(1, selected[0]);
    TEST_ASSERT_EQUAL(2, selected[1]);
}

static void test_password_list_selection_out_of_the_list(void) {
    add_three_passwords();
    choices_per_page = 2;
    selector_callback = spy_selector;
    display_password_list();

    list_content()->contentActionCallback(0, 1, 1);
    TEST_ASSERT_EQUAL(0, selected_count);
}

static void test_password_list_selection_without_action(void) {
    add_three_passwords();
    display_password_list();
    const size_t count = screen_count;

    list_content()->contentActionCallback(0, 0, 0);
    TEST_ASSERT_EQUAL(count, screen_count);
}

/* ----------------------------------------------------------------------- */
/* Show and type                                                           */
/* ----------------------------------------------------------------------- */

static void test_show_password(void) {
    add_three_passwords();
    display_password_list();

    show_password_cb(1);
    TEST_ASSERT_EQUAL(1, generation_calls);
    TEST_ASSERT_NOT_NULL(generation_out);
    TEST_ASSERT_EQUAL_STRING("Your Password", screen.title);
    const nbgl_content_t *content = list_content();
    TEST_ASSERT_EQUAL(INFOS_LIST, content->type);
    TEST_ASSERT_EQUAL(1, content->content.infosList.nbInfos);
    TEST_ASSERT_EQUAL_STRING("bravo", content->content.infosList.infoTypes[0]);
    const char *password = content->content.infosList.infoContents[0];
    TEST_ASSERT_EQUAL_STRING(GENERATED_PASSWORD, password);

    // Leaving the page wipes the password
    screen.quit();
    TEST_ASSERT_EQUAL(SCREEN_NAVIGABLE_CONTENT, screen.type);
    TEST_ASSERT_EACH_EQUAL_CHAR('\0', password, PASSWORD_MAX_SIZE + 1);
}

static void test_show_password_generation_failure(void) {
    add_three_passwords();
    display_password_list();
    generation_result = false;

    show_password_cb(0);
    check_status("COULD NOT GENERATE\nTHE PASSWORD", false);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
    TEST_ASSERT_EACH_EQUAL_CHAR('\0', generation_out, PASSWORD_MAX_SIZE + 1);
}

// The list is reset once a password is shown
static void test_show_password_without_list(void) {
    show_password_cb(0);
    TEST_ASSERT_EQUAL(0, generation_calls);
    TEST_ASSERT_EQUAL(SCREEN_NAVIGABLE_CONTENT, screen.type);
}

static void test_clear_displayed_password(void) {
    add_three_passwords();
    display_password_list();
    show_password_cb(2);
    const char *password = list_content()->content.infosList.infoContents[0];

    clear_displayed_password();
    TEST_ASSERT_EACH_EQUAL_CHAR('\0', password, PASSWORD_MAX_SIZE + 1);
}

static void test_type_password(void) {
    add_three_passwords();
    display_password_list();

    type_password_cb(0);
    TEST_ASSERT_EQUAL(1, generation_calls);
    TEST_ASSERT_NULL(generation_out);
    check_status("PASSWORD HAS\nBEEN WRITTEN", true);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
}

static void test_type_password_generation_failure(void) {
    add_three_passwords();
    display_password_list();
    generation_result = false;

    type_password_cb(0);
    check_status("COULD NOT GENERATE\nTHE PASSWORD", false);
}

/* ----------------------------------------------------------------------- */
/* Deletion                                                                */
/* ----------------------------------------------------------------------- */

static void test_delete_password(void) {
    add_three_passwords();
    display_password_list();

    confirm_password_deletion_cb(1);
    TEST_ASSERT_EQUAL(SCREEN_STATIC_REVIEW, screen.type);
    TEST_ASSERT_EQUAL_STRING("Confirm the deletion\nof password\n'bravo'", screen.long_press.text);
    TEST_ASSERT_EQUAL_STRING("Hold to confirm", screen.long_press.longPressText);

    screen.choice(true);
    check_status("PASSWORD HAS\nBEEN DELETED", true);
    TEST_ASSERT_EQUAL(2, N_storage.metadata_count);
    TEST_ASSERT_FALSE(nickname_exists("bravo", strlen("bravo")));
    TEST_ASSERT_TRUE(nickname_exists("alpha", strlen("alpha")));
    TEST_ASSERT_TRUE(nickname_exists("charlie", strlen("charlie")));
}

static void test_delete_password_rejected(void) {
    add_three_passwords();
    display_password_list();
    confirm_password_deletion_cb(0);

    screen.choice(false);
    TEST_ASSERT_EQUAL(SCREEN_NAVIGABLE_CONTENT, screen.type);
    TEST_ASSERT_EQUAL(3, N_storage.metadata_count);
}

// The database is emptied between the selection and the confirmation
static void test_delete_password_failure(void) {
    add_three_passwords();
    display_password_list();
    confirm_password_deletion_cb(0);
    N_storage_rw.metadata_count = 0;

    screen.choice(true);
    check_status("Erase Error\nDatabase already empty", false);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
}

static void test_delete_all_passwords(void) {
    add_three_passwords();

    confirm_all_passwords_deletion();
    TEST_ASSERT_EQUAL(SCREEN_STATIC_REVIEW, screen.type);
    TEST_ASSERT_EQUAL_STRING("Confirm the deletion\nof all passwords (3)", screen.long_press.text);

    screen.choice(true);
    check_status("PASSWORDS HAVE\nBEEN DELETED", true);
    TEST_ASSERT_EQUAL(0, N_storage.metadata_count);
}

static void test_delete_all_passwords_empty(void) {
    confirm_all_passwords_deletion();
    check_status("No passwords available", false);
}

/* ----------------------------------------------------------------------- */
/* Creation                                                                */
/* ----------------------------------------------------------------------- */

static void type_nickname(const char *nickname) {
    TEST_ASSERT_EQUAL(SCREEN_KEYBOARD, screen.type);
    strcpy(screen.keyboard.entryBuffer, nickname);
    screen.keyboard.confirmationParams.onButtonCallback();
}

static void test_create_password_keyboard(void) {
    display_create_pwd();
    TEST_ASSERT_EQUAL(SCREEN_KEYBOARD, screen.type);
    TEST_ASSERT_EQUAL_STRING("New password nickname", screen.title);
    TEST_ASSERT_EQUAL_STRING("Create password", screen.keyboard.confirmationParams.buttonText);
    TEST_ASSERT_EQUAL(MAX_NICKNAME_LEN, screen.keyboard.entryMaxLen);
    TEST_ASSERT_EQUAL_STRING("", screen.keyboard.entryBuffer);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
}

static void test_create_password(void) {
    display_create_pwd();
    type_nickname("delta");

    check_status("NEW PASSWORD\nCREATED", true);
    TEST_ASSERT_EQUAL(1, N_storage.metadata_count);
    TEST_ASSERT_TRUE(nickname_exists("delta", strlen("delta")));
}

static void test_create_password_empty_nickname(void) {
    display_create_pwd();
    type_nickname("");

    check_status("The nickname\ncan't be empty", false);
    TEST_ASSERT_EQUAL_PTR(display_create_pwd, screen.quit);
    TEST_ASSERT_EQUAL(0, N_storage.metadata_count);
}

static void test_create_password_existing_nickname(void) {
    add_three_passwords();
    display_create_pwd();
    type_nickname("bravo");

    check_status("This nickname\nalready exists", false);
    TEST_ASSERT_EQUAL_PTR(display_create_pwd, screen.quit);
    TEST_ASSERT_EQUAL(3, N_storage.metadata_count);
}

static void test_create_password_failure(void) {
    N_storage_rw.charset_options = SEPARATOR_SETS;
    display_create_pwd();
    type_nickname("delta");

    check_status("Write Error\nSeparators alone are too weak", false);
    TEST_ASSERT_EQUAL_PTR(display_choice_page, screen.quit);
    TEST_ASSERT_EQUAL(0, N_storage.metadata_count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_password_list_empty);
    RUN_TEST(test_password_list_shows_every_nickname);
    RUN_TEST(test_password_list_sorted);
    RUN_TEST(test_password_list_up_to_the_threshold);
    RUN_TEST(test_password_list_slices);
    RUN_TEST(test_password_list_back_to_a_slice_on_another_page);
    RUN_TEST(test_password_list_slice_labels_by_position);
    RUN_TEST(test_password_list_slice_labels);
    RUN_TEST(test_password_list_stops_at_a_corrupted_record);
    RUN_TEST(test_password_list_selection_is_absolute);
    RUN_TEST(test_password_list_selection_out_of_the_list);
    RUN_TEST(test_password_list_selection_without_action);
    RUN_TEST(test_show_password);
    RUN_TEST(test_show_password_generation_failure);
    RUN_TEST(test_show_password_without_list);
    RUN_TEST(test_clear_displayed_password);
    RUN_TEST(test_type_password);
    RUN_TEST(test_type_password_generation_failure);
    RUN_TEST(test_delete_password);
    RUN_TEST(test_delete_password_rejected);
    RUN_TEST(test_delete_password_failure);
    RUN_TEST(test_delete_all_passwords);
    RUN_TEST(test_delete_all_passwords_empty);
    RUN_TEST(test_create_password_keyboard);
    RUN_TEST(test_create_password);
    RUN_TEST(test_create_password_empty_nickname);
    RUN_TEST(test_create_password_existing_nickname);
    RUN_TEST(test_create_password_failure);
    return UNITY_END();
}
