#include "types.h"

// Same globals as app_main.c, with the persistent storage writable by nvm_write()
internalStorage_t N_storage_real;
app_state_t app_state;
volatile unsigned int G_led_status;
