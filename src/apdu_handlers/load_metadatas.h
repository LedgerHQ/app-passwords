#ifndef __LOAD_METADATAS_H__
#define __LOAD_METADATAS_H__

#include "stdint.h"
#include "types.h"

#define P1_LAST_CHUNK 0xFF

int load_metadatas(uint8_t p1, uint8_t p2, const buf_t *input);

#endif