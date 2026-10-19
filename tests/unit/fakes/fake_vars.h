#pragma once

#include "globals.h"

// Writable view of the persistent storage, defined without const in fake_vars.c
#define N_storage_rw (*(internalStorage_t *) &N_storage_real)
