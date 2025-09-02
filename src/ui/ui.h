#pragma once

#include "ux.h"
#include "types.h"

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_HOME C_home_passwords_14px
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_HOME C_app_passwords_64px
#endif

/**
 * @brief prototype of function to be called when an page of settings is double-pressed
 * @param page page index (0->(nb_pages-1))
 */
typedef void (*pwd_actionCallback_t)(const size_t index);
extern pwd_actionCallback_t selector_callback;

void ui_idle();
void ui_request_user_approval(message_pair_t *msg);

void display_choice_page(void);
void display_password_list(void);
void display_create_pwd(void);
void confirm_all_passwords_deletion(void);
void confirm_password_deletion_cb(const size_t index);
void show_password_cb(const size_t index);
void type_password_cb(const size_t index);
