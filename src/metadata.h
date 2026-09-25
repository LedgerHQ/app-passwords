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

void reset_metadatas(void);

/*
 * Check a complete MAX_METADATAS-byte database image held in RAM, without touching NVM.
 */
error_type_t validate_metadata_image(const uint8_t *image);

/*
 * Replace the live database with a validated image, then compact it.
 */
error_type_t commit_metadata_image(const uint8_t *image);

/*
 * Restore transaction marker. begin/end bracket the commit of a restored image into the live
 * database; abort drops a half-written database and clears the marker.
 * `metadata_restore_in_progress()` is true when a commit never completed, including across a
 * reboot.
 */
void begin_metadata_restore(void);
void end_metadata_restore(void);
void abort_metadata_restore(void);
bool metadata_restore_in_progress(void);
error_type_t erase_metadata(uint32_t offset);
uint32_t find_free_metadata(void);
uint32_t get_metadata(uint32_t nth);
error_type_t compact_metadata();
