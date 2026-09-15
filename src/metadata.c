#include <string.h>

#include "metadata.h"
#include "globals.h"

/*
 * Reads the record header at `offset`, checking that the whole record fits inside
 * `N_storage.metadatas` *before* any of its fields is dereferenced. Every parsing loop goes
 * through this so a crafted or truncated database cannot walk past the end of the array.
 *
 * On success `*total_len` is the number of bytes the record occupies, or 0 for the
 * end-of-database terminator, and `*kind` is META_NONE or META_ERASED.
 */
static error_type_t metadata_record_at(uint32_t offset, uint32_t *total_len, uint8_t *kind) {
    if (offset >= MAX_METADATAS) {
        // Ran off the end without meeting a terminator.
        return ERR_CORRUPTED_METADATA;
    }

    const uint8_t datalen = N_storage.metadatas[offset];
    if (datalen == 0) {
        *total_len = 0;
        *kind = META_NONE;
        return OK;
    }

    // A record always carries its charset byte, and never more than a full nickname.
    if (datalen > MAX_METANAME) {
        return ERR_METADATA_ENTRY_TOO_BIG;
    }
    const uint32_t total = (uint32_t) datalen + 2;
    if (total > (MAX_METADATAS - offset)) {
        return ERR_CORRUPTED_METADATA;
    }

    const uint8_t record_kind = N_storage.metadatas[offset + 1];
    if ((record_kind != META_NONE) && (record_kind != META_ERASED)) {
        return ERR_CORRUPTED_METADATA;
    }

    *total_len = total;
    *kind = record_kind;
    return OK;
}

error_type_t write_metadata(uint8_t *data, uint8_t dataSize) {
    if (dataSize > MAX_METANAME) {
        dataSize = MAX_METANAME;
    }
    error_type_t err = compact_metadata();
    if (err) {
        return err;
    }
    /* MAX_METADATAS has room for more short entries than the fixed-size UI list arrays can
     * hold, so the entry count has its own limit on top of the free-space check below. */
    if (N_storage.metadata_count >= MAX_METADATA_COUNT) {
        return ERR_NO_MORE_SPACE_AVAILABLE;
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

error_type_t override_metadatas(size_t offset, void *ptr, size_t size) {
    /* Re-check the destination here rather than trust the transfer offset the caller tracks:
     * the first test also keeps the second one from wrapping around on a size_t. */
    if ((offset > sizeof(N_storage.metadatas)) || (size > sizeof(N_storage.metadatas) - offset)) {
        return ERR_NO_MORE_SPACE_AVAILABLE;
    }
    nvm_write((void *) &N_storage.metadatas[offset], ptr, size);
    return OK;
}

void begin_metadata_restore(void) {
    const uint8_t marker = 1;
    nvm_write((void *) &N_storage.restore_in_progress, (void *) &marker, sizeof(marker));
}

void end_metadata_restore(void) {
    nvm_write((void *) &N_storage.restore_in_progress, NULL, sizeof(N_storage.restore_in_progress));
}

bool metadata_restore_in_progress(void) {
    return N_storage.restore_in_progress != 0;
}

void abort_metadata_restore(void) {
    // The transfer left a mix of the old and the new image behind. There is no way to tell
    // them apart, so drop the whole database rather than leave it to be parsed later.
    reset_metadatas();
    end_metadata_restore();
}

void reset_metadatas(void) {
    nvm_write((void *) N_storage.metadatas, NULL, sizeof(N_storage.metadatas));
    nvm_write((void *) &N_storage.metadata_count, 0, sizeof(N_storage.metadata_count));
}

error_type_t erase_metadata(uint32_t offset) {
    if (N_storage.metadata_count == 0) {
        return ERR_NO_METADATA;
    }
    // The offset comes from the UI selection, so validate the record before writing into it.
    uint32_t entry_len;
    uint8_t kind;
    const error_type_t err = metadata_record_at(offset, &entry_len, &kind);
    if ((err != OK) || (entry_len == 0)) {
        return ERR_CORRUPTED_METADATA;
    }

    size_t metadata_count = N_storage.metadata_count - 1;
    unsigned char m = META_ERASED;
    nvm_write((void *) &N_storage.metadatas[offset + 1], &m, sizeof(N_storage.metadatas[0]));
    // Marking the entry erased leaves its nickname readable in flash until the next compaction
    // moves other entries over it, so wipe the data block now.
    nvm_write((void *) &N_storage.metadatas[offset + 2], NULL, entry_len - 2);
    nvm_write((void *) &N_storage.metadata_count,
              &metadata_count,
              sizeof(N_storage.metadata_count));
    return OK;
}

uint32_t find_free_metadata(void) {
    uint32_t offset = 0;
    for (;;) {
        uint32_t entry_len;
        uint8_t kind;
        if (metadata_record_at(offset, &entry_len, &kind) != OK) {
            // Corrupt storage has no usable free space; callers treat this as "database full"
            // rather than handing out an offset derived from unparsable bytes.
            return MAX_METADATAS;
        }
        if (entry_len == 0) {
            return offset;
        }
        offset += entry_len;
    }
}

uint32_t get_metadata(uint32_t nth) {
    uint32_t offset = 0;
    for (;;) {
        uint32_t entry_len;
        uint8_t kind;
        if (metadata_record_at(offset, &entry_len, &kind) != OK) {
            return UINT32_MAX;
        }
        if (entry_len == 0) {
            return UINT32_MAX;  // end of file
        }
        if (kind != META_ERASED) {
            if (nth == 0) {
                return offset;
            }
            nth--;
        }
        offset += entry_len;
    }
}

error_type_t compact_metadata() {
    uint32_t offset = 0;
    uint32_t shift_offset = 0;
    /* Offset 0 is a legitimate destination (it is where the live entries move when the very
     * first entry is the erased one), so `shift_offset` cannot double as the "nothing erased
     * yet" marker: that conflation used to leave an erased first entry in place and counted
     * it as live below. Track the state explicitly instead. */
    bool shifting = false;
    uint8_t copy_buffer[2 + 1 + MAX_METANAME];

    for (;;) {
        uint32_t entry_len;
        uint8_t kind;
        const error_type_t err = metadata_record_at(offset, &entry_len, &kind);
        if (err != OK) {
            return err;
        }
        if (entry_len == 0) {
            break;  // end of the database
        }
        if (kind == META_ERASED) {
            if (!shifting) {
                shift_offset = offset;
                shifting = true;
            }
        } else if (shifting) {
            // Move the live entry down over the space the erased ones left behind.
            memcpy(copy_buffer, (const void *) METADATA_PTR(offset), entry_len);
            nvm_write((void *) &N_storage.metadatas[shift_offset], copy_buffer, entry_len);
            shift_offset += entry_len;
        }
        offset += entry_len;
    }
    // declare that the remaining space is free
    if (shifting) {
        copy_buffer[0] = 0;
        copy_buffer[1] = META_NONE;
        nvm_write((void *) &N_storage.metadatas[shift_offset], copy_buffer, 2);
        // Wipe what the compaction vacated between the new terminator and the old end of the
        // database, so the nicknames that used to live there are not left in the slack space.
        if (offset > (shift_offset + 2)) {
            nvm_write((void *) &N_storage.metadatas[shift_offset + 2],
                      NULL,
                      offset - (shift_offset + 2));
        }
    }
    // count metadatas
    offset = 0;
    size_t count = 0;
    for (;;) {
        uint32_t entry_len;
        uint8_t kind;
        const error_type_t err = metadata_record_at(offset, &entry_len, &kind);
        if (err != OK) {
            return err;
        }
        if (entry_len == 0) {
            break;
        }
        offset += entry_len;
        count++;
        /* Refuse a database holding more entries than the UI list can address, rather than
         * committing a count the password list cannot honour. */
        if (count > MAX_METADATA_COUNT) {
            return ERR_NO_MORE_SPACE_AVAILABLE;
        }
    }
    nvm_write((void *) &N_storage.metadata_count,
              (void *) &count,
              sizeof(N_storage.metadata_count));
    return OK;
}
