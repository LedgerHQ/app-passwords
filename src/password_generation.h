/*******************************************************************************
 *   Password Manager application
 *   (c) 2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#ifndef PASSWORD_GENERATION_H

#define PASSWORD_GENERATION_H

#include <stdint.h>
#include "ctr_drbg.h"

typedef enum {
    UPPERCASE = 1,
    LOWERCASE = 2,
    NUMBERS = 4,
    MINUS = 8,
    UNDERLINE = 16,
    SPACE = 32,
    SPECIAL = 64,
    BRACKETS = 128,

    ALL_SETS = 0xFF,
} setmask_t;

#define NUM_SETS 8

uint32_t generate_password(mbedtls_ctr_drbg_context *drbg,
                           setmask_t setMask,
                           const uint8_t *minFromSet,
                           uint8_t *out,
                           uint32_t size);

#endif
