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
#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include "ds1307.h"  // RTC lib for ds1307 clock chip
#include "spi.h"  // SPI bus for mcu intercommunication (mcu1/mcu2) and SD card (mcu2)
#include "usart_0.h"  // UART0 for Wifly WiFi communication (mcu1) / GPS (mcu2)
#include "usart_1.h"  // UART1 for shell and debugging (mcu1/mcu2)
#include "util.h"     // util functions
#include "venus.h"    // GPS lib for Venus 638FLPx SkyTraq board
#include "wifi.h"     // communicating over UART0 <-> WiFi (mcu1)

// command interface types
typedef enum {
  CMD_IF_IC,    // SPI intercommunication mcu1/mcu2
  CMD_IF_WIFI,  // uart0 WiFi Wifly mcu1
  CMD_IF_GPS,   // uart0 GPS mcu2
  CMD_IF_RTC,   // I2C mcu2
  CMD_IF_SD,    // SPI SD Card mcu2
  CMD_IF_EXT,   // External (can be from WiFi or serial console)
  CMD_IF_DEBUG  // uart1 mcu1/mcu2
} tCMDInterface;

// status bytes
#define CMD_START 0x01  // start flag of command
#define CMD_STOP 0x02   // stop flag of command
#define CMD_NOOP 0x03   // out buffer empty, just send No Operation bytes

// command bytes, single letter, to keep into the ascii set
#define ALL_CMDS "abcdefghijkl"
#define CMD_STATE 'a'
#define CMD_STATS 'b'
#define CMD_DATA 'c'
#define CMD_GPS 'd'
#define CMD_RTC 'e'
#define CMD_SENSE 'f'
#define CMD_SETTINGS 'g'
#define CMD_LOG 'h'
#define CMD_REBOOT 'i'
#define CMD_PINCODE 'j'
#define CMD_SOUND 'k'
#define CMD_MSG 'l'

typedef enum { IDLE, BUSY, START, CMD } t_cmd_status;

void command_init(void);
void command_process(void);
char *command_util_btob(uint8_t x, char *bstr);
#ifdef EASYRIDER_MCU2
uint8_t command_trigger_stats(tCMDInterface cmd_interface);
uint8_t command_trigger_state(tCMDInterface cmd_interface);
uint8_t command_trigger_sound(uint8_t status, tCMDInterface cmd_interface);
uint8_t command_trigger_gps(tCMDInterface cmd_interface);
void command_util_get_timestamp(void);
#endif
#ifdef EASYRIDER_MCU1
void command_dispatch(const char *data);
#endif

#endif
