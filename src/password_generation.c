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
#include "metadata.h"
#include "password.h"
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

/* Draws a value below `modulo` by rejection sampling. Returns false if no value could be drawn.
 * With `uniform` false (generation version 0), `candidate == rng_limit` is accepted, which favours
 * residue 0; that boundary is kept so version 0 passwords do not change. */
static bool rng_u8_modulo(mbedtls_ctr_drbg_context *drbg,
                          uint8_t modulo,
                          bool uniform,
                          uint8_t *out) {
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
    } while (uniform ? (candidate >= rng_limit) : (candidate > rng_limit));
    // PRINTF("r:%02X ", candidate);
    *out = candidate % modulo;
    return true;
}

static bool shuffle_array(mbedtls_ctr_drbg_context *drbg,
                          uint8_t *buffer,
                          uint32_t size,
                          bool uniform) {
    uint32_t i;
    // Nothing to shuffle for an empty array, and guard against the unsigned
    // underflow of `size - 1` (which would index buffer way out of bounds).
    if (size == 0) {
        return true;
    }
    for (i = size - 1; i > 0; i--) {
        uint8_t index;
        if (!rng_u8_modulo(drbg, i + 1, uniform, &index)) {
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
                   bool uniform,
                   uint8_t *out,
                   uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        uint8_t index;
        if (!rng_u8_modulo(drbg, setSize, uniform, &index)) {
            return false;
        }
        out[i] = set[index];
    }
    return true;
}

/* A group of character sets the password must draw at least `min` characters from. */
typedef struct set_group_s {
    uint8_t sets;
    uint8_t min;
} set_group_t;

/* Version 0: one group per set, in set order. */
static const set_group_t GROUPS_V0[] = {
    {UPPERCASE, 1},
    {LOWERCASE, 1},
    {NUMBERS, 1},
    {MINUS, 0},
    {UNDERLINE, 0},
    {SPACE, 1},
    {SPECIAL, 0},
    {BRACKETS, 0},
};

/* Version 1: separators and special characters each form one group, and the draw is uniform. */
static const set_group_t GROUPS_V1[] = {
    {UPPERCASE, 1},
    {LOWERCASE, 1},
    {NUMBERS, 1},
    {MINUS | UNDERLINE | SPACE, 1},
    {SPECIAL | BRACKETS, 1},
};

/* Concatenates the sets of `setMask` into `chars`, in set order. Returns the length, or 0 if
 * they do not fit. */
static uint32_t collect_sets(uint8_t setMask, uint8_t *chars, uint32_t charsSize) {
    uint32_t length = 0;
    for (uint32_t i = 0; i < NUM_SETS; i++) {
        if ((setMask & (1 << i)) == 0) {
            continue;
        }
        const uint8_t *set = (const uint8_t *) PIC(SETS[i]);
        const uint32_t setSize = strlen((const char *) set);
        if (setSize > charsSize - length) {
            return 0;
        }
        memcpy(chars + length, set, setSize);
        length += setSize;
    }
    return length;
}

/* `out` must have room for `size` characters plus a NUL terminator. On failure nothing is
 * guaranteed about its contents and no terminator is written, so the caller must wipe it. */
bool generate_password(mbedtls_ctr_drbg_context *drbg,
                       setmask_t setMask,
                       uint8_t version,
                       uint8_t *out,
                       uint32_t size) {
    const set_group_t *groups;
    uint32_t nbGroups;
    bool uniform;
    uint8_t setChars[100];
    uint32_t outOffset = 0;

    switch (version) {
        case META_VERSION_0:
            groups = (const set_group_t *) PIC(GROUPS_V0);
            nbGroups = ARRAYLEN(GROUPS_V0);
            uniform = false;
            break;
        case META_VERSION_1:
            if (charsets_are_weak(setMask) || !password_length_fits_charsets(size, setMask)) {
                return false;
            }
            groups = (const set_group_t *) PIC(GROUPS_V1);
            nbGroups = ARRAYLEN(GROUPS_V1);
            uniform = true;
            break;
        default:
            return false;
    }

    // Draw the minimum of each enabled group first, from the enabled sets of that group only.
    for (uint32_t i = 0; i < nbGroups; i++) {
        const uint8_t groupSets = setMask & groups[i].sets;
        const uint8_t min = groups[i].min;
        if ((groupSets == 0) || (min == 0)) {
            continue;
        }
        const uint32_t groupSize = collect_sets(groupSets, setChars, sizeof(setChars));
        if ((groupSize == 0) || (outOffset + min > size)) {
            return false;
        }
        if (!sample(drbg, setChars, groupSize, uniform, out + outOffset, min)) {
            return false;
        }
        outOffset += min;
    }

    // Then fill the rest from all the enabled sets.
    const uint32_t setCharsOffset = collect_sets(setMask, setChars, sizeof(setChars));
    if (setCharsOffset == 0 || setCharsOffset >= sizeof(setChars)) {
        return false;
    }

    // PRINTF("chars from: %.*H\n", setCharsOffset, setChars);

    if (!sample(drbg, setChars, setCharsOffset, uniform, out + outOffset, size - outOffset)) {
        return false;
    }
    // PRINTF("selected: %.*H\n", size, out);
    if (!shuffle_array(drbg, out, size, uniform)) {
        return false;
    }
    out[size] = '\0';
    return true;
}
