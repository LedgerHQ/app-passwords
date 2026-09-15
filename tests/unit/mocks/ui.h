#pragma once

#include "types.h"

// src/ui/ui.h pulls in the SDK's ux.h and the app's glyphs, neither of which exists on the
// host. The APDU handlers only need these two entry points, so this mock stands in for the
// whole header. src/ui is not on the unit-test include path, so `#include "ui.h"` from a
// handler resolves here.
void ui_idle(void);
void ui_request_user_approval(message_pair_t *msg);
