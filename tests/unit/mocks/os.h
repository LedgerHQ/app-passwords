#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PRINTF printf
#define THROW(X) return
#define EXCEPTION 1


bool bolos_ux_mnemonic_check(const unsigned char* buffer, unsigned int length) {
    const char* expected_mnemonic = "list of random words which actually are the mnemonic";
    printf("Comparing strings under size '%d'\n", length);
    printf(" - expected: '%s'\n", expected_mnemonic);
    printf(" - given:    '%s'\n", buffer);
    return (strncmp(expected_mnemonic, (const char *)buffer, length) == 0);
}
