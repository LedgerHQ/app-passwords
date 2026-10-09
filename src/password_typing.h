#pragma once

#include <stdint.h>

#include "password_generation.h"

bool type_password(uint8_t *data,
                   uint32_t dataSize,
                   uint8_t *out,
                   setmask_t setMask,
                   uint8_t version,
                   uint32_t size);
