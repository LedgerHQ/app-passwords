#pragma once

#include <stdint.h>

// Pulled in via globals.h. The real SDK header declares the IO buffer used
// to talk to the SE; on the host side we only need the symbol so password.c
// can be compiled and linked. The buffer itself is supplied by the test code.
extern uint8_t G_io_seproxyhal_spi_buffer[];
