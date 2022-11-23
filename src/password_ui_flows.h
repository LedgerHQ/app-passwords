#pragma once

#include "ux.h"
#include "types.h"

extern const message_pair_t ERR_MESSAGES[];

void ui_idle();
void ui_request_user_approval(message_pair_t *msg);
void ui_error(message_pair_t err);
