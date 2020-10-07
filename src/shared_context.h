#ifndef __SHARED_CONTEXT_H__
#define __SHARED_CONTEXT_H__

#include "os.h"
#include "cx.h"

#define METADATA_DATALEN(offset) N_storage.metadatas[offset]
#define METADATA_KIND(offset) N_storage.metadatas[offset+1]
#define METADATA_SETS(offset) N_storage.metadatas[offset+2]
#define METADATA_PWLEN(offset) (N_storage.metadatas[offset]-1)
#define METADATA_PW(offset) (&N_storage.metadatas[offset+3])

#define META_NONE 0x00
#define META_ERASED 0xFF

#define N_storage (*(volatile internalStorage_t*) PIC(&N_storage_real))

typedef struct internalStorage_t {
#define STORAGE_MAGIC 0xDEAD1337
    uint32_t magic;
    uint32_t keyboard_layout;
    /**
    * A metadata in memory is represented by 1 byte of size (l), 1 byte of type (to disable it if required), 1 byte to select char sets, l bytes of user seed
    */
    size_t metadata_count;
    uint8_t metadatas[MAX_METADATAS];
} internalStorage_t;

extern const internalStorage_t N_storage_real;

extern volatile unsigned int G_led_status;
extern uint32_t currentMetadataOffset;
extern uint8_t firstMetadataFound;

void app_init();

#endif