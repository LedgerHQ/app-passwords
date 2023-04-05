#pragma once

#include "ux.h"
#include "types.h"

extern const message_pair_t ERR_MESSAGES[];

void ui_idle();
void ui_request_user_approval(message_pair_t *msg);
void ui_error(message_pair_t err);

#define UPPERCASE_BITFLAG   1
#define LOWERCASE_BITFLAG   2
#define NUMBERS_BITFLAG     4
#define BARS_BITFLAG        8 | 16 | 32
#define EXT_SYMBOLS_BITFLAG 64 | 128
