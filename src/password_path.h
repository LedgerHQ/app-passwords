#pragma once

#include <stdint.h>

#define DERIVE_PASSWORD_PATH    0x80505744
#define PASSWORD_PATH_LEN       9
#define PASSWORD_PATH_DIGEST_SZ 32

/*
 * Build the hardened BIP32 path of a password from the SHA-256 digest of its nickname:
 * DERIVE_PASSWORD_PATH, then each big-endian 32-bit word of the digest with its hardened bit set.
 */
void password_path_from_digest(const uint8_t digest[PASSWORD_PATH_DIGEST_SZ],
                               uint32_t path[PASSWORD_PATH_LEN]);
