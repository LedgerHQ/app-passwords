#ifndef _PASSWORD_UI_H_
#define _PASSWORD_UI_H_

#include "ux.h"

void ui_idle(void);

#define UPPERCASE_BITFLAG 1
#define LOWERCASE_BITFLAG 2
#define NUMBERS_BITFLAG 4
#define BARS_BITFLAG 8|16|32
#define EXT_SYMBOLS_BITFLAG 64|128

#endif