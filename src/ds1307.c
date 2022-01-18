/*
 *
 *  Copyright (C) Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
#include "ds1307.h"

// BCD to decimal byte
static uint8_t ds1307_bcd2dec(uint8_t val);
// decimal byte to BCD
static uint8_t ds1307_dec2bcd(uint8_t val);

// BCD to decimal byte
static uint8_t ds1307_bcd2dec(uint8_t val) { return val - 6 * (val >> 4); }

// decimal byte to BCD
static uint8_t ds1307_dec2bcd(uint8_t val) { return val + 6 * (val / 10); }

// start the oscillator by clearing the HALT bit
static void ds1307_start_osc(void);

// start the RTC's oscillator by clearing the CH bit
// this function only needs to be called once, after physically installing the
// RTC component and powering it along with a battery-backup line,
// but we'll call it every time a new date/time set command is triggered
// in case the battery was pulled out
void ds1307_start_osc() {
  i2c_start(DS1307_ADDRESS + I2C_WRITE);
  i2c_write(DS1307_SECONDS);  // select first register
  i2c_write(0);               // clear whole register byte
  i2c_stop();
}

// initializes the i2c interface to the DS1307
void ds1307_init() { i2c_init(); }

// set date/time
void ds1307_set_date(RTCDate *date) {
  ds1307_start_osc();
  i2c_start_wait(DS1307_ADDRESS + I2C_WRITE);
  i2c_write(DS1307_SECONDS);  // select register first
  i2c_write(ds1307_dec2bcd(date->seconds));
  i2c_write(ds1307_dec2bcd(date->minutes));
  i2c_write(ds1307_dec2bcd(date->hours));
  i2c_write(ds1307_dec2bcd(date->weekday));
  i2c_write(ds1307_dec2bcd(date->day));
  i2c_write(ds1307_dec2bcd(date->month));
  i2c_write(ds1307_dec2bcd(date->year));
  i2c_write(0x00);  // write zero's to last control register
  i2c_stop();
}

// read date/time
void ds1307_get_date(RTCDate *date) {
  i2c_start_wait(DS1307_ADDRESS + I2C_WRITE);
  i2c_write(DS1307_SECONDS);  // select register first
  i2c_rep_start(DS1307_ADDRESS + I2C_READ);
  date->seconds = ds1307_bcd2dec(i2c_readAck() & 0x7F);  // mask out CH bit
  date->minutes = ds1307_bcd2dec(i2c_readAck());
  date->hours = ds1307_bcd2dec(i2c_readAck() &
                               0x3F);  // mask last 2 bits for 24hr mode only
  date->weekday = ds1307_bcd2dec(i2c_readAck());
  date->day = ds1307_bcd2dec(i2c_readAck());
  date->month = ds1307_bcd2dec(i2c_readAck());
  date->year = ds1307_bcd2dec(i2c_readNak());
  i2c_stop();
}
