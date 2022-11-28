#include "globals.h"


void init_storage() {
    if (N_storage.magic == STORAGE_MAGIC) {
        // already initialized
        return;
    }
    uint32_t tmp = STORAGE_MAGIC;
    nvm_write((void *) &N_storage.magic, (void *) &tmp, sizeof(uint32_t));
    tmp = 0;
    nvm_write((void *) &N_storage.press_enter_after_typing,
              (void *) &tmp,
              sizeof(N_storage.press_enter_after_typing));
    nvm_write((void *) &N_storage.keyboard_layout,
              (void *) &tmp,
              sizeof(N_storage.keyboard_layout));
    nvm_write((void *) &N_storage.metadata_count,
              (void *) &tmp,
              sizeof(N_storage.metadata_count));
    nvm_write((void *) N_storage.metadatas, (void *) &tmp, 2);
#if defined(TARGET_FATSTACKS)
    #include "options.h"
    init_charset_options();
#endif
}
