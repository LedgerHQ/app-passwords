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

#ifndef PASSWORD_GENERATION_H

#define PASSWORD_GENERATION_H

#include <stdint.h>
#include "ctr_drbg.h"

uint32_t generate_password(mbedtls_ctr_drbg_context *drbg, uint32_t numLetters,
                           uint32_t numSpecials, uint32_t numNumbers,
                           uint8_t *out);

#endif
