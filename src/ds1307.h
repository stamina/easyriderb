/*
 *
 *  Copyright (C) 2014 Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
#ifndef DS1307_H_INCLUDED
#define DS1307_H_INCLUDED

#include <stdint.h>
#include "i2c.h"  // I2C (2-wire) protocol

// register addresses
#define DS1307_SECONDS 0x00  // 1-60
#define DS1307_MINUTES 0x01  // 1-60
#define DS1307_HOURS 0x02    // 1-23 (using 24hr format, register setting)
#define DS1307_DAY 0x03      // 1-7  monday - sunday
#define DS1307_DATE 0x04     // 1-31
#define DS1307_MONTH 0x05    // 1-12
#define DS1307_YEAR 0x06     // 00-99
#define DS1307_CONTROL \
  0x07  // square wave pin control byte, not used since its physically not
        // attached

//  the i2c slave base address
#define DS1307_ADDRESS 0xD0  // 7bit which gets a  read/write bit (LSB) attached

// date/time struct
typedef struct {
  // this value is from the internal MCU timer (so asynchronous from the RTC,
  // some syncing/overflow correction is applied)
  volatile uint16_t milliseconds;
  // below values are read from the RTC
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t weekday;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} RTCDate;

// initializes the i2c interface to the DS1307
void ds1307_init(void);
// read date/time
void ds1307_get_date(RTCDate *date);
// set date/time
void ds1307_set_date(RTCDate *date);

#endif
