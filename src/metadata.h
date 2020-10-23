#ifndef __METADATA_H__
#define __METADATA_H__

#include "stdint.h"

uint8_t write_metadata(uint8_t *data, uint8_t dataSize);
void reset_metadatas(void);
uint8_t erase_metadata(uint32_t offset);
uint32_t find_free_metadata(void);
uint32_t find_next_metadata(uint32_t offset);
uint32_t get_metadata(uint32_t nth);
uint32_t find_previous_metadata(uint32_t offset);

#endif