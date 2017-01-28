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

#ifndef BINARY_KEYBOARD_H
#define BINARY_KEYBOARD_H

#define BKB_TYPE_BUFF_CAP (MAX_METANAME - 1 > 256 ? 256 : MAX_METANAME - 1)

extern char bkb_type_buff[BKB_TYPE_BUFF_CAP];

void bkb_init();
void bkb_choose_left();
void bkb_choose_right();
void bkb_draw();
void bkb_animate(unsigned int elapsed);
void bkb_display_ready();
unsigned int bkb_get_type_buff_size();

#endif
