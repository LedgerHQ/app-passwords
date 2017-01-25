/*
 * License for the BOLOS User Interface project, originally found here:
 * https://github.com/parkerhoyes/bolos-user-interface
 *
 * Copyright (C) 2016 Parker Hoyes <contact@parkerhoyes.com>
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 *    that you wrote the original software. If you use this software in a
 *    product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_COMIC_SANS_MS_20
#include "bui_font_comic_sans_ms_20.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_LUCIDA_CONSOLE_8
#include "bui_font_lucida_console_8.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_LUCIDA_CONSOLE_15
#include "bui_font_lucida_console_15.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_BOLD_13
#include "bui_font_open_sans_bold_13.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_BOLD_21
#include "bui_font_open_sans_bold_21.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_EXTRABOLD_11
#include "bui_font_open_sans_extrabold_11.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_13
#include "bui_font_open_sans_light_13.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_14
#include "bui_font_open_sans_light_14.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_16
#include "bui_font_open_sans_light_16.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_20
#include "bui_font_open_sans_light_20.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_21
#include "bui_font_open_sans_light_21.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_32
#include "bui_font_open_sans_light_32.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_REGULAR_11
#include "bui_font_open_sans_regular_11.inc"
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_SEMIBOLD_18
#include "bui_font_open_sans_semibold_18.inc"
#endif

static const bui_font_t * const bui_fonts[] = {
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_COMIC_SANS_MS_20
	&bui_font_comic_sans_ms_20,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_LUCIDA_CONSOLE_8
	&bui_font_lucida_console_8,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_LUCIDA_CONSOLE_15
	&bui_font_lucida_console_15,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_BOLD_13
	&bui_font_open_sans_bold_13,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_BOLD_21
	&bui_font_open_sans_bold_21,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_EXTRABOLD_11
	&bui_font_open_sans_extrabold_11,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_13
	&bui_font_open_sans_light_13,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_14
	&bui_font_open_sans_light_14,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_16
	&bui_font_open_sans_light_16,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_20
	&bui_font_open_sans_light_20,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_21
	&bui_font_open_sans_light_21,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_LIGHT_32
	&bui_font_open_sans_light_32,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_REGULAR_11
	&bui_font_open_sans_regular_11,
#else
	NULL,
#endif
#if !defined BUI_FONT_CHOOSE || defined BUI_FONT_INCLUDE_OPEN_SANS_SEMIBOLD_18
	&bui_font_open_sans_semibold_18,
#else
	NULL,
#endif
};
