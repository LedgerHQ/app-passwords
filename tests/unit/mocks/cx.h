#pragma once

#include <stdint.h>

// Pulled in transitively by ctr_drbg.h (-> password_generation.h ->
// password_typing.h -> password.c). The production header from the SDK
// declares cryptographic primitives (cx_aes_*, CX_LAST, CX_ENCRYPT, ...).
// password.c / metadata.c never reference any of them. The only piece needed
// at parse time is the cx_aes_key_t type, used as a struct field type in
// mbedtls_ctr_drbg_context. A dummy typedef satisfies the compiler without
// pulling in the rest of the SDK crypto layer.
typedef struct {
    uint8_t bytes[64];
} cx_aes_key_t;
