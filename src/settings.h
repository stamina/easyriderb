/*
 *
 *  Copyright (C) 2014 Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ROM_SYSTEM_NAME "easyrider"
#define ROM_POWER_CYCLES 0
#define ROM_PINCODE "0000"
#define ROM_SD_LOG 0
#define ROM_SW_VERSION 100
#define ROM_HW_VERSION "rev B"
#define ROM_P_SENSES_ACTIVE 65535
#define ROM_D_SENSES_ACTIVE 0
#define ROM_ALARM_SETTLE_TIME 800
#define ROM_INDICATOR_SOUND 1
#define ROM_BLINK_SPEED 9765
#define ROM_ALARM_COUNTER 6
#define ROM_ALARM_TRIGGER 18
#define ROM_ALARM_TRIGGER_COUNTER 4
#define ROM_ALARM_THRES_MIN 140
#define ROM_ALARM_TRHES_MAX 550
#define ROM_STARTUP_SOUND 255

// settings saved in EEPROM
typedef struct {
  char system_name[10];   // contains the string: "easyrider", not changeable,
                          // used to check for correctly filled in EEPROM
                          // settings
  uint16_t power_cycles;  // number of startups of mcu, default: 0
  char pincode[5];  // // authentication for app over Wifi, default: "0000"
  uint8_t sd_log;   // flag to log to microSD card, default: 0
  uint16_t
      sw_version;  // firmware version (software), e.g. 435 -> version 4.3.5
  char hw_version[10];         // board version (hardware), e.g. "rev b"
  uint16_t p_senses_active;    // default: 65535 (0xFFFF), all active
  uint16_t d_senses_active;    // default: 0 (0x0000), all inactive
  uint16_t alarm_settle_time;  // default 800: 5ms ticks, so 4 secs
  uint8_t indicator_sound;     // default 1: beep when indicators blink
  uint16_t
      blink_speed;  // default 9765: 1 tick is 0.0000512 secs -> 0.5secs on/off
  uint8_t alarm_counter;  // default 6 times: 3 times on and 3 times off
  uint8_t alarm_trigger;  // default 18: voltage offset 18 * 5V/1024 parts
  uint8_t alarm_trigger_counter;  // default 4: we need 4 times a postive alarm
                                  // trigger, each measurement is done TIMER1
                                  // (0.5s) tick
  uint16_t alarm_thres_min;       // default 140: voltage minimum 0.685
  uint16_t alarm_thres_max;       // default 550: voltage maximum 2.685
  uint8_t startup_sound;  // default 255: random, [0-252] index of music array,
                          // no sound: 254, beep only: 253
} tSettings;

void read_settings(tSettings *settings);
void update_settings(tSettings *settings);
void reset_settings(void);

#endif
