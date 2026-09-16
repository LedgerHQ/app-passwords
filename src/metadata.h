#pragma once

#include <stdint.h>

#include "error.h"
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
error_type_t override_metadatas(size_t offset, void *ptr, size_t size);

/*
 * Zero the metadata region from `offset` to the end. Used to drop whatever the previous
 * database left past the bytes a restore actually delivered.
 */
void clear_metadatas_from(size_t offset);

void reset_metadatas(void);

/*
 * Restore transaction marker. begin/end bracket the writes a LOAD_METADATAS transfer makes
 * into the live database; abort drops a half-written database and clears the marker.
 * `metadata_restore_in_progress()` is true when a previous transfer never reached its end,
 * including across a reboot.
 */
void begin_metadata_restore(void);
void end_metadata_restore(void);
void abort_metadata_restore(void);
bool metadata_restore_in_progress(void);
error_type_t erase_metadata(uint32_t offset);
uint32_t find_free_metadata(void);
uint32_t get_metadata(uint32_t nth);
error_type_t compact_metadata();
