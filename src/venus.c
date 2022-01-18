/*
 *
 *  Copyright (C) 2015 Bas Brugman
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
#include "venus.h"

uint16_t SWAP16(uint16_t x) { return ((x&0xff)<<8 | (x>>8)); }
uint32_t SWAP32(uint32_t x) { return ((x&0xff)<<24 | ((x&0xff00)<<8) | ((x&0xff0000)>>8) | ((x&0xff000000)>>24)); }

uint8_t gps_read(uint8_t c);
void gps_csv(void);
void process_location(void);

// one time setup to configure GPS Venus chip
void gps_setup() {
}

// swap little-endian to big-endian for better number processing
void process_location() {
  gps_msg.location.gps_week = SWAP16(gps_msg.location.gps_week);
  gps_msg.location.gps_tow = SWAP32(gps_msg.location.gps_tow);
  gps_msg.location.latitude = SWAP32(gps_msg.location.latitude);
  gps_msg.location.longitude = SWAP32(gps_msg.location.longitude);
  gps_msg.location.ellipsoid_alt = SWAP32(gps_msg.location.ellipsoid_alt);
  gps_msg.location.sealevel_alt = SWAP32(gps_msg.location.sealevel_alt);
  gps_msg.location.gdop = SWAP16(gps_msg.location.gdop);
  gps_msg.location.pdop = SWAP16(gps_msg.location.pdop);
  gps_msg.location.hdop = SWAP16(gps_msg.location.hdop);
  gps_msg.location.vdop = SWAP16(gps_msg.location.vdop);
  gps_msg.location.tdop = SWAP16(gps_msg.location.tdop);
  gps_msg.location.ecef_coor.x = SWAP32(gps_msg.location.ecef_coor.x);
  gps_msg.location.ecef_coor.y = SWAP32(gps_msg.location.ecef_coor.y);
  gps_msg.location.ecef_coor.z = SWAP32(gps_msg.location.ecef_coor.z);
  gps_msg.location.ecef_vel.x = SWAP32(gps_msg.location.ecef_vel.x);
  gps_msg.location.ecef_vel.y = SWAP32(gps_msg.location.ecef_vel.y);
  gps_msg.location.ecef_vel.z = SWAP32(gps_msg.location.ecef_vel.z);
}

// reads and processes the incoming binary messages byte by byte (from GPS -> mcu via uart)
uint8_t gps_read(uint8_t c) {
  static uint8_t state = 0;
  static uint8_t checksum = 0;
  static uint8_t payload_idx = 0;

  switch(state) {
    case 0: // look for fixed first byte 
      if (c == 0xA0) state++;
      break;
    case 1: // look for fixed 2nd byte 
      if (c == 0xA1) {
        state++;
      } else {
        state = 0; // reset to start state
      }
      break;
    case 2: // read payload high byte, not used since payload will never exceed 0xFF
      state++;
      break;
    case 3: // read payload low byte
      gps_msg.length = c;  
      state++;
      break;
    case 4: // read message ID
      gps_msg.id = checksum = c;
      payload_idx = 0; 
      if (--gps_msg.length > 0) {
        state++; // go to state which reads payload message
      } else {
        state = 6; // no message besides msg ID, skip read state
      }
      break;
    case 5: // read bytes of the payload
      if (payload_idx < VENUS_MAX_PAYLOAD) {
        gps_msg.body[payload_idx] = c;
      }
      checksum ^= c; // update checksum
      payload_idx++;
      if (payload_idx >= gps_msg.length) state++; // done reading msg bytes
      break;
    case 6: // checksum check 
      if(c == checksum) { 
        state++; 
      } else { // checksum failed, go to starting state
        state = 0; 
        return VENUS_BADCR; 
      }
      break;
    case 7:
      if (c == 0x0d) state++; // first end of msg byte found: CR
      break;
    case 8: 
      state = 0;
      if (gps_msg.id == VENUS_GPS_LOCATION) { // post-process binary message to a usable format
        process_location();
        gps_csv();
      }
      return (c == 0x0A) ? ( (payload_idx <= VENUS_MAX_PAYLOAD) ? VENUS_OK : VENUS_CLIPPED) : VENUS_INCOMPLETE; 
    default: 
      state = 0; 
      return VENUS_INCOMPLETE;
  }
  return VENUS_MORE;
}

// fills the current relevant GPS data into a command-separated string:
// fix number
// satellites number
// latitude
// longitude
// sealevel
// ECEF x y z velocities
void gps_csv() {
  memset((void*)gps_ascii, 0x30, VENUS_MAX_ASCII); // prefill 0's
  uint8_t idx = 0;
  utoa(gps_msg.location.fix, &gps_ascii[idx], 10);            // #total chars: 1
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 2
  utoa(gps_msg.location.sv_count, &gps_ascii[idx], 10);       // #total chars: 34
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 5
  ltoa(gps_msg.location.latitude, &gps_ascii[idx], 10);       // #total chars: 6789 10 11 12 13 14 15 16
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 17
  ltoa(gps_msg.location.longitude, &gps_ascii[idx], 10);      // #total chars: 18 19 20 21 22 23 24 25 26 27 28
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 29
  ultoa(gps_msg.location.sealevel_alt, &gps_ascii[idx], 10);  // #total chars: 30 31 32 33 34 35 36 37 38 39
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 40
  ltoa(gps_msg.location.ecef_vel.x, &gps_ascii[idx], 10);     // #total chars: 41 42 43 44 45 46 47 48 49 50 51
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 52
  ltoa(gps_msg.location.ecef_vel.y, &gps_ascii[idx], 10);     // #total chars: 53 54 55 56 57 58 59 60 61 62 63
  idx = strlen(gps_ascii);
  gps_ascii[idx] = ',';                                       // #total chars: 64
  ltoa(gps_msg.location.ecef_vel.z, &gps_ascii[idx], 10);     // #total chars: 65 66 67 68 69 70 71 72 73 74 75
}

// main process function externally called
void gps_process() {
  uint8_t status;
  if (uart_available_0()) {
    status = gps_read(uart_get_0());
    if (status == VENUS_OK) { // complete message received
      gps_status++;
      if (!gps_status) gps_status = 1;
#ifdef EASY_TRACE
      uart_put_str_1("VENUS_OK\r\n");
#endif
    }
  }
}

