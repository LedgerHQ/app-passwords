#include "metadata.h"

#include "shared_context.h"

uint8_t write_metadata(uint8_t *data, uint8_t dataSize) {
    uint8_t tmp[2];
    size_t metadata_count = N_storage.metadata_count + 1;
    uint32_t offset = find_free_metadata();
    if ((offset + dataSize + 2 + 2) > MAX_METADATAS) {
        return 0;
    }
    nvm_write((void *) &N_storage.metadatas[offset + 2], data, dataSize);
    tmp[0] = 0;
    tmp[1] = META_NONE;
    nvm_write((void *) &N_storage.metadatas[offset + 2 + dataSize], tmp, 2);
    tmp[0] = dataSize;
    tmp[1] = META_NONE;
    nvm_write((void *) &N_storage.metadatas[offset], tmp, 2);
    nvm_write((void *) &N_storage.metadata_count, &metadata_count, 4);
    return 1;
}

void reset_metadatas(void) {
    nvm_write((void *) N_storage.metadatas, NULL, sizeof(N_storage.metadatas));
}

uint8_t erase_metadata(uint32_t offset) {
    if (N_storage.metadata_count == 0) {
        return 0;
    }
    size_t metadata_count = N_storage.metadata_count - 1;
    unsigned char m = META_ERASED;
    nvm_write((void *) &N_storage.metadatas[offset + 1], &m, 1);
    nvm_write((void *) &N_storage.metadata_count, &metadata_count, 4);
    return 1;
}

uint32_t find_free_metadata(void) {
    uint32_t offset = 0;
    while ((METADATA_DATALEN(offset) != 0) && (offset < MAX_METADATAS)) {
        offset += METADATA_DATALEN(offset) + 2;
    }
    return offset;
}

uint32_t find_next_metadata(uint32_t offset) {
    if (!firstMetadataFound) {
        firstMetadataFound = (METADATA_DATALEN(0) != 0);
        return 0;
    }
    offset += METADATA_DATALEN(offset) + 2;
    for (;;) {
        if (METADATA_DATALEN(offset) == 0) {
            return 0;  // end of file
        }
        if (METADATA_KIND(offset) != META_ERASED) {
            return offset;  // next entry
        }
        offset += METADATA_DATALEN(offset) + 2;
    }
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
        offset += METADATA_DATALEN(offset) + 2;
    }
}

uint32_t find_previous_metadata(uint32_t offset) {
    uint32_t searchOffset = 0;
    uint32_t lastValidOffset = 0;
    if (offset == 0) {
        return 0;
    }
    for (;;) {
        if (METADATA_KIND(searchOffset) != META_ERASED) {
            lastValidOffset = searchOffset;
        }
        searchOffset += METADATA_DATALEN(searchOffset) + 2;
        if (searchOffset == offset) {
            break;
        }
    }
    return lastValidOffset;
}