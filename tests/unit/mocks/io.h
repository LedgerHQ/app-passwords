#pragma once

#include <stddef.h>
#include <stdint.h>

// The SDK's lib_standard_app/io.h, reduced to what the APDU handlers use. There it defines
// io_send_sw() / io_send_response_pointer() as `static inline` wrappers over the IO layer;
// here they are plain declarations so each test can supply its own capturing stub.
//
// IO_APDU_BUFFER_SIZE keeps the SDK's formula (OS_IO_BUFFER_SIZE + 1, itself
// OS_IO_SEPH_BUFFER_SIZE, defined by the unit-test CMakeLists) so MAX_PAYLOAD_SIZE in
// handlers.h works out to the same chunk size as on device.
#define IO_APDU_BUFFER_SIZE (OS_IO_SEPH_BUFFER_SIZE + 1)

extern uint8_t G_io_apdu_buffer[IO_APDU_BUFFER_SIZE];

int io_send_sw(uint16_t sw);
int io_send_response_pointer(const uint8_t *ptr, size_t size, uint16_t sw);
