/*
 *
 *  Copyright (C) 2014 Bas Brugman
 *  http://www.visionnaire.nl
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include "util.h"

// byte to binary ascii representation
// used to convert the 2 state bytes to a bit string
char* command_util_btob(uint8_t x, char* bstr) {
  uint8_t z;
  for (z = 128; z > 0; z >>= 1) {
    strcat(bstr, ((x & z) == z) ? "1" : "0");
  }
  return bstr;
}

// converts uint8_t to ascii number representation with
// left padding of zero's to form a fixed size number
void fixed_ascii_uint8(char* buffer, uint8_t nr) {
  sprintf(buffer, "%03u", nr);
}
