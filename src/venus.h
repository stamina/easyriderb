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
#ifndef VENUS_H_INCLUDED
#define VENUS_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "usart_0.h"  // UART0 for GPS (mcu2)
#include "usart_1.h"  // UART1 for DEBUG

void gps_process(void);
void gps_setup(void);

#define VENUS_MAX_PAYLOAD 255
#define VENUS_MAX_ASCII 80

#define VENUS_GPS_LOCATION 0xA8  // GPS message id for binary data output

char gps_ascii[VENUS_MAX_ASCII];  // GPS ascii format, comma-seperated
uint8_t gps_status;  // 0 = no binary message yet or # of msgs received (rolls
                     // over at 255 to 1)

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} xyz32_t;

typedef struct {
  uint8_t fix;             // fix mode: 0 = no fix, 1,2,3 = 2D,3D,3D+DGPS
  uint8_t sv_count;        // # of satellites in view
  uint16_t gps_week;       // gps week nr
  uint32_t gps_tow;        // gps time of week
  int32_t latitude;        // latitude coors, N > 0, S < 0
  int32_t longitude;       // longitude coors, E > 0, W < 0
  uint32_t ellipsoid_alt;  // height above ellipsoid
  uint32_t sealevel_alt;   // height above sea level
  uint16_t gdop, pdop, hdop, vdop,
      tdop;           // Geometric, Position, Horiz, Vert, Time dilutions
  xyz32_t ecef_coor;  // ECEF x,y,z coordinates
  xyz32_t ecef_vel;   // ECEF x,y,z velocities
} venus_location;

// messages from the GPS unit over uart0
typedef struct {
  uint8_t length;  // payload length;
  uint8_t id;      // message id
  // NOTE: only filling the body array, but this space aligns with the location
  // struct in the same union, so is directly accessible
  union {
    uint8_t body[VENUS_MAX_PAYLOAD];  // raw messages
    venus_location location;  // specifically post-processed location structure
  };
} venus_message;

venus_message gps_msg;

#define VENUS_OK 0     // full msg read successful
#define VENUS_BADCR 1  // checksum failure
#define VENUS_CLIPPED \
  2                   // message buffer too small, response payload was clipped
#define VENUS_MORE 3  // more data remaining
#define VENUS_INCOMPLETE \
  4  // not a correct end of msg found, message was incomplete

#endif
