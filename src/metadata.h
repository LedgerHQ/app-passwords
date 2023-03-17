#pragma once

#include <stdint.h>

#include "types.h"

#define METADATA_PTR(offset)       (&N_storage.metadatas[offset])
#define METADATA_TOTAL_LEN(offset) (METADATA_DATALEN(offset) + 2)
#define METADATA_DATALEN(offset)   N_storage.metadatas[offset]  // charsets(1) + pwd seed(n)
#define METADATA_KIND(offset)      N_storage.metadatas[offset + 1]
#define METADATA_SETS(offset)      N_storage.metadatas[offset + 2]
/* even if the database is corrupted, this guarantees we never overflow buffers of size
 * MAX_METANAME */
#define METADATA_NICKNAME_LEN(offset) ((METADATA_DATALEN(offset) - 1) % (MAX_METANAME + 1))
#define METADATA_NICKNAME(offset)     (&N_storage.metadatas[offset + 3])

#define META_NONE   0x00
#define META_ERASED 0xFF

error_type_t write_metadata(uint8_t *data, uint8_t dataSize);

/*
 * Write a given amount of data on metadatas, at the given offset
 * Used to load metadata from APDUs
 */
void override_metadatas(uint8_t offset, void *ptr, size_t size);

void reset_metadatas(void);
error_type_t erase_metadata(uint32_t offset);
uint32_t find_free_metadata(void);
uint32_t get_metadata(uint32_t nth);
error_type_t compact_metadata();
