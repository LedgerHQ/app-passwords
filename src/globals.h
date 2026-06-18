#pragma once

#include "os.h"
#include "os_io_seproxyhal.h"
#include "types.h"

extern const internalStorage_t N_storage_real;
extern app_state_t app_state;
extern volatile unsigned int G_led_status;

#define CLA       0xE0
#define N_storage (*(volatile internalStorage_t *) PIC(&N_storage_real))

// Returns true if the storage was freshly initialized (first run), false if it
// was already initialized on a previous boot.
bool init_storage(void);
