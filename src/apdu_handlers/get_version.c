/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
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
 *****************************************************************************/

#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "get_version.h"
#include "io.h"
#include "sw.h"
#include "types.h"

int get_version(uint8_t p1, uint8_t p2, const buf_t *input) {
    if (p1 != 0 || p2 != 0) {
        return send_sw(SW_WRONG_P1P2);
    }

    const buf_t buf = {.bytes =
                           (uint8_t[3]){
                               (uint8_t) MAJOR_VERSION,  //
                               (uint8_t) MINOR_VERSION,  //
                               (uint8_t) PATCH_VERSION,  //
                           },                            //
                       .size = 3};

    return send(&buf, SW_OK);
}
