#include "password_path.h"

void password_path_from_digest(const uint8_t digest[PASSWORD_PATH_DIGEST_SZ],
                               uint32_t path[PASSWORD_PATH_LEN]) {
    path[0] = DERIVE_PASSWORD_PATH;
    for (uint32_t i = 0; i < PASSWORD_PATH_LEN - 1; i++) {
        path[i + 1] = 0x80000000u | ((uint32_t) digest[4 * i] << 24) |
                      ((uint32_t) digest[4 * i + 1] << 16) | ((uint32_t) digest[4 * i + 2] << 8) |
                      ((uint32_t) digest[4 * i + 3]);
    }
}
