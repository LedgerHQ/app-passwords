#include "shared_context.h"
#include "string.h"

const internalStorage_t N_storage_real;
volatile unsigned int G_led_status;

uint32_t currentMetadataOffset;
uint8_t firstMetadataFound;

void app_init(){
    if (N_storage.magic != STORAGE_MAGIC) {
        uint32_t tmp = STORAGE_MAGIC;
        nvm_write((void*)&N_storage.magic, (void *)&tmp, sizeof(uint32_t));
        tmp = 0;
        nvm_write((void*)&N_storage.press_enter_after_typing, (void *)&tmp, sizeof(N_storage.press_enter_after_typing));
        nvm_write((void*)&N_storage.keyboard_layout, (void *)&tmp, sizeof(N_storage.keyboard_layout));
        nvm_write((void*)&N_storage.metadata_count, (void *)&tmp, sizeof(N_storage.metadata_count));
        nvm_write((void*)N_storage.metadatas, (void *)&tmp, 2);
    }
    currentMetadataOffset = 0;
    firstMetadataFound = 0;
}