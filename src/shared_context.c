#include "shared_context.h"
#include "string.h"

const internalStorage_t N_storage_real;
volatile unsigned int G_led_status;

uint32_t currentMetadataOffset;
uint8_t firstMetadataFound;

void app_init(){
    if (N_storage.magic != STORAGE_MAGIC) {
        uint32_t magic;
        uint8_t dummy[4];
        magic = STORAGE_MAGIC;
        nvm_write((void*)&N_storage.magic, (void *)&magic, sizeof(uint32_t));
        os_memset(dummy, 0, 4);
        nvm_write((void*)&N_storage.keyboard_layout, (void *)&dummy, sizeof(N_storage.keyboard_layout));
        nvm_write((void*)&N_storage.metadata_count, (void *)&dummy, sizeof(N_storage.metadata_count));
        nvm_write((void*)N_storage.metadatas, (void *)&dummy, 2);
    }
    currentMetadataOffset = 0;
    firstMetadataFound = 0;
}