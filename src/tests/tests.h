#pragma once

#include <stdint.h>
#include "types.h"

typedef enum {
    GENERATE_PASSWORD = 0x01,
} test_cmd_e;

int test_dispatcher(uint8_t p1, uint8_t p2, const buf_t *input);

#if defined(POPULATE)
/* Create the demo passwords selected by the POPULATE level (see the Makefile). */
void populate_passwords(void);
#endif  // POPULATE
