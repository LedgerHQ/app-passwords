#pragma once

#include "os.h"
#include "os_io_seproxyhal.h"
#include "types.h"

extern const internalStorage_t N_storage_real;
extern app_state_t app_state;
extern volatile unsigned int G_led_status;

#define OFFSET_CLA   0
#define OFFSET_INS   1
#define OFFSET_P1    2
#define OFFSET_P2    3
#define OFFSET_LC    4
#define OFFSET_CDATA 5

#define CLA       0xE0
#define N_storage (*(volatile internalStorage_t*) PIC(&N_storage_real))
