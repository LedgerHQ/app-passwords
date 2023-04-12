#pragma once

#include <stdint.h>
#include <ux.h>

#include "types.h"

#if !defined(TARGET_STAX)
void io_seproxyhal_display(const bagl_element_t *element);
#endif

uint8_t io_event(uint8_t channel);
uint16_t io_exchange_al(uint8_t channel, uint16_t tx_len);
int recv(void);
int send(const buf_t *buf, uint16_t sw);
int send_sw(uint16_t sw);
