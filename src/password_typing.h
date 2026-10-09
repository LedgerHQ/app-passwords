#pragma once

#include <stdint.h>

#include "password_generation.h"

/* Generates the `size`-character password of `data` (1 to PASSWORD_MAX_SIZE). Writes it into
 * `out`, which must hold `size` + 1 bytes, or types it on the host keyboard if `out` is NULL. */
bool type_password(uint8_t *data,
                   uint32_t dataSize,
                   uint8_t *out,
                   setmask_t setMask,
                   uint8_t version,
                   uint32_t size);
