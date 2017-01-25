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

#include "os.h"
#include "cx.h"
#include "password_generation.h"

static const uint8_t LETTERS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const uint8_t NUMBERS[] = "0123456789";
static const uint8_t SPECIALS[] = "@&-():;!?,/.';";

uint8_t rng_u8_modulo(mbedtls_ctr_drbg_context *drbg, uint8_t modulo) {
    uint32_t rng_max = 256 % modulo;
    uint32_t rng_limit = 256 - rng_max;
    uint8_t candidate;
    do {
        if (mbedtls_ctr_drbg_random(drbg, &candidate, 1) != 0) {
            THROW(EXCEPTION);
        }
    } while (candidate > rng_limit);
    return (candidate % modulo);
}

void shuffle_array(mbedtls_ctr_drbg_context *drbg, uint8_t *buffer,
                   uint32_t size) {
    uint32_t i;
    for (i = size - 1; i > 0; i--) {
        uint32_t index = rng_u8_modulo(drbg, i + 1);
        uint8_t tmp = buffer[i];
        buffer[i] = buffer[index];
        buffer[index] = tmp;
    }
}

uint32_t generate_password(mbedtls_ctr_drbg_context *drbg, uint32_t numLetters,
                           uint32_t numSpecials, uint32_t numNumbers,
                           uint8_t *out) {
    uint8_t tmp[60];
    uint32_t outOffset = 0;
    os_memmove(tmp, LETTERS, sizeof(LETTERS));
    shuffle_array(drbg, tmp, sizeof(LETTERS) - 1);
    os_memmove(out + outOffset, tmp, numLetters);
    outOffset += numLetters;
    os_memmove(tmp, SPECIALS, sizeof(SPECIALS));
    shuffle_array(drbg, tmp, sizeof(SPECIALS) - 1);
    os_memmove(out + outOffset, tmp, numSpecials);
    outOffset += numSpecials;
    os_memmove(tmp, NUMBERS, sizeof(NUMBERS));
    shuffle_array(drbg, tmp, sizeof(NUMBERS) - 1);
    os_memmove(out + outOffset, tmp, numNumbers);
    outOffset += numNumbers;
    shuffle_array(drbg, out, outOffset);
    out[outOffset] = '\0';
    return outOffset;
}
