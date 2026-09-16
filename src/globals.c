#include "globals.h"
#include "options.h"

bool init_storage() {
    if (N_storage.magic == STORAGE_MAGIC) {
        // already initialized
        return false;
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
    nvm_write((void *) &N_storage.metadata_count, (void *) &tmp, sizeof(N_storage.metadata_count));
    nvm_write((void *) &N_storage.restore_in_progress,
              (void *) &tmp,
              sizeof(N_storage.restore_in_progress));
    // Clear the whole region, not just the terminator: whatever the flash held before is
    // otherwise still there behind the logical end of the database.
    nvm_write((void *) N_storage.metadatas, NULL, sizeof(N_storage.metadatas));
    init_charset_options();
    return true;
}
