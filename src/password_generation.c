/*******************************************************************************
 *   Password Manager application
 *   (c) 2017-2023 Ledger SAS
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

#include "cx.h"
#include "password_generation.h"

static const char *SETS[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ",  // 26
                             "abcdefghijklmnopqrstuvwxyz",  // 26
                             "0123456789",                  // 10
                             "-",
                             "_",
                             " ",
                             "\"#$%&'*+,./:;=?!@\\^`|~",  // 22
                             "[]{}()<>",                  // 8
                             NULL};

/* Draws a value below `modulo` by rejection sampling. Returns false if no value could be drawn,
 * so callers can unwind and wipe their buffers instead of unwinding through an exception.
 *
 * The draw is NOT uniform, deliberately. The rejection boundary is off by one: `candidate ==
 * rng_limit` is accepted, so residue 0 keeps one preimage more than the others -- 3 against 2
 * for the 95-character alphabet, a factor of 1.5. Moduli that divide 256 are unaffected, their
 * rng_limit being unreachable for a uint8_t.
 *
 * Rejecting `>= rng_limit` would fix it, and must not be done here: the draw would consume a
 * different DRBG byte, shifting every later character, and about one password in seven would
 * come out different. They are derived deterministically and stored nowhere, so users would
 * lose access to those accounts. The bias costs 0.035 bits out of 131 over a 20-character
 * password, which is the cheaper end of that trade. Accepted as a known risk in V-003;
 * revisiting it needs a per-entry version byte in the metadata format, so that old entries
 * keep this boundary and new ones get the correct one.
 *
 * The vectors in tests/functional/tests_vectors.py pin the current output. */
static bool rng_u8_modulo(mbedtls_ctr_drbg_context *drbg, uint8_t modulo, uint8_t *out) {
    if (modulo == 0) {
        return false;
    }
    uint32_t rng_max = 256 % modulo;
    uint32_t rng_limit = 256 - rng_max;
    uint8_t candidate = 0;
    do {
        if (mbedtls_ctr_drbg_random(drbg, &candidate, 1) != 0) {
            return false;
        }
    } while (candidate > rng_limit);
    // PRINTF("r:%02X ", candidate);
    *out = candidate % modulo;
    return true;
}

static bool shuffle_array(mbedtls_ctr_drbg_context *drbg, uint8_t *buffer, uint32_t size) {
    uint32_t i;
    // Nothing to shuffle for an empty array, and guard against the unsigned
    // underflow of `size - 1` (which would index buffer way out of bounds).
    if (size == 0) {
        return true;
    }
    for (i = size - 1; i > 0; i--) {
        uint8_t index;
        if (!rng_u8_modulo(drbg, i + 1, &index)) {
            return false;
        }
        uint8_t tmp = buffer[i];
        buffer[i] = buffer[index];
        buffer[index] = tmp;
    }
    return true;
}

/* Sample from set with replacement */
static bool sample(mbedtls_ctr_drbg_context *drbg,
                   const uint8_t *set,
                   uint32_t setSize,
                   uint8_t *out,
                   uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        uint8_t index;
        if (!rng_u8_modulo(drbg, setSize, &index)) {
            return false;
        }
        out[i] = set[index];
    }
    return true;
}

/* `out` must have room for `size` characters plus a NUL terminator. On failure nothing is
 * guaranteed about its contents and no terminator is written, so the caller must wipe it. */
bool generate_password(mbedtls_ctr_drbg_context *drbg,
                       setmask_t setMask,
                       const uint8_t *minFromSet,
                       uint8_t *out,
                       uint32_t size) {
    uint8_t setChars[100];
    uint32_t setCharsOffset = 0;
    uint32_t outOffset = 0;
    uint32_t i;

    for (i = 0; setMask && i < NUM_SETS; i++, setMask >>= 1) {
        if (setMask & 1) {
            const uint8_t *set = (const uint8_t *) PIC(SETS[i]);
            uint32_t setSize = strlen((const char *) set);
            if (setSize > sizeof(setChars) - setCharsOffset) {
                return false;
            }
            memcpy(setChars + setCharsOffset, set, setSize);
            setCharsOffset += setSize;

            // for at least requested minimum chars from that set
            if (minFromSet[i] > 0) {
                if (outOffset + minFromSet[i] > size) {
                    return false;
                }
                if (!sample(drbg, set, setSize, out + outOffset, minFromSet[i])) {
                    return false;
                }
                outOffset += minFromSet[i];
            }
        }
    }

    if (setMask || setCharsOffset == 0 || setCharsOffset >= sizeof(setChars)) {
        return false;
    }

    // PRINTF("chars from: %.*H\n", setCharsOffset, setChars);

    if (!sample(drbg, setChars, setCharsOffset, out + outOffset, size - outOffset)) {
        return false;
    }
    // PRINTF("selected: %.*H\n", size, out);
    if (!shuffle_array(drbg, out, size)) {
        return false;
    }
    out[size] = '\0';
    return true;
}
