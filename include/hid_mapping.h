/*******************************************************************************
 *   Password Manager application
 *   (c) 2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

#define HID_MAPPING_H
#define SHIFT_KEY 0x02
#define ALT_KEY   0x04

#include <stdint.h>

enum hid_mapping_e {
    HID_MAPPING_QWERTY = 1,
    HID_MAPPING_QWERTY_INTL = 2,
    HID_MAPPING_AZERTY = 3,
};
typedef enum hid_mapping_e hid_mapping_t;

void map_char(hid_mapping_t mapping, uint8_t key, uint8_t *out);
