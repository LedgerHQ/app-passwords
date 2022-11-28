#include <string.h>

#include "metadata.h"
#include "globals.h"

error_type_t write_metadata(uint8_t * data, uint8_t dataSize) {
    if (dataSize > MAX_METANAME) {
        dataSize = MAX_METANAME;
    }
    error_type_t err = compact_metadata();
    if (err) {
        return err;
    }
    uint32_t offset = find_free_metadata();
    if ((offset + dataSize + 2 + 2) > MAX_METADATAS) {
        return ERR_NO_MORE_SPACE_AVAILABLE;
    }
    nvm_write((void *) &N_storage.metadatas[offset + 2], data, dataSize);
    uint8_t tmp[2];
    tmp[0] = 0;
    tmp[1] = META_NONE;
    nvm_write((void *) &N_storage.metadatas[offset + 2 + dataSize], tmp, 2);
    tmp[0] = dataSize;
    tmp[1] = META_NONE;
    nvm_write((void *) &N_storage.metadatas[offset], tmp, 2);
    size_t metadata_count = N_storage.metadata_count + 1;
    nvm_write((void *) &N_storage.metadata_count, &metadata_count, 4);
    return OK;
}

void override_metadatas(uint8_t offset, void* ptr, size_t size) {
    nvm_write((void *) N_storage.metadatas + offset, ptr, size);
}

void reset_metadatas(void) {
    nvm_write((void *) N_storage.metadatas, NULL, sizeof(N_storage.metadatas));
}

error_type_t erase_metadata(uint32_t offset) {
    if (N_storage.metadata_count == 0) {
        return ERR_NO_METADATA;
    }
    size_t metadata_count = N_storage.metadata_count - 1;
    unsigned char m = META_ERASED;
    nvm_write((void *) &N_storage.metadatas[offset + 1], &m, 1);
    nvm_write((void *) &N_storage.metadata_count, &metadata_count, 4);
    return OK;
}

uint32_t find_free_metadata(void) {
    uint32_t offset = 0;
    while ((METADATA_DATALEN(offset) != 0) && (offset < MAX_METADATAS)) {
        offset += METADATA_TOTAL_LEN(offset);
    }
    return offset;
}

uint32_t get_metadata(uint32_t nth) {
    unsigned int offset = 0;
    for (;;) {
        if (METADATA_DATALEN(offset) == 0) {
            return -1UL;  // end of file
        }
        if (METADATA_KIND(offset) != META_ERASED) {
            if (nth == 0) {
                return offset;
            }
            nth--;
        }
        offset += METADATA_TOTAL_LEN(offset);
    }
}

error_type_t compact_metadata() {
    uint32_t offset = 0;
    uint32_t shift_offset = 0;
    uint8_t copy_buffer[2 + 1 + MAX_METANAME];
    while ((METADATA_DATALEN(offset) != 0) && (offset < MAX_METADATAS)) {
        if (METADATA_TOTAL_LEN(offset) >= sizeof(copy_buffer)) {
            return ERR_METADATA_ENTRY_TOO_BIG;
        }
        switch (METADATA_KIND(offset)) {
            case META_NONE:
                if (shift_offset != 0) {
                    memcpy(copy_buffer,
                           (const void *) METADATA_PTR(offset),
                           METADATA_TOTAL_LEN(offset));
                    nvm_write((void *) &N_storage.metadatas[shift_offset],
                              copy_buffer,
                              METADATA_TOTAL_LEN(offset));
                    offset += METADATA_TOTAL_LEN(shift_offset);
                    shift_offset += METADATA_TOTAL_LEN(shift_offset);
                } else {
                    offset += METADATA_TOTAL_LEN(offset);
                }
                break;
            case META_ERASED:
                shift_offset = shift_offset == 0 ? offset : shift_offset;
                offset += METADATA_TOTAL_LEN(offset);
                break;

            default:
                return ERR_CORRUPTED_METADATA;
        }
    }
    if (shift_offset >= MAX_METADATAS || offset >= MAX_METADATAS) {
        return ERR_NO_MORE_SPACE_AVAILABLE;
    }
    // declare that the remaining space is free
    if (shift_offset != 0) {
        copy_buffer[0] = 0;
        copy_buffer[1] = META_NONE;
        nvm_write((void *) &N_storage.metadatas[shift_offset], copy_buffer, 2);
    }
    // count metadatas
    offset = 0;
    size_t count = 0;
    while ((METADATA_DATALEN(offset) != 0) && (offset < MAX_METADATAS)) {
        offset += METADATA_TOTAL_LEN(offset);
        count++;
    }
    nvm_write((void *) &N_storage.metadata_count,
              (void *) &count,
              sizeof(N_storage.metadata_count));
    return OK;
}
