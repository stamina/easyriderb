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
#include "spi.h"

// Initial setup function per mcu
// MCU1 is always the slave
// MCU2 can be turned OFF or set to master
void spi_init(tSPIState role, uint8_t speed,  uint8_t mode, uint8_t bitorder, uint8_t interrupt) {
#ifdef EASYRIDER_MCU1
  if (role == SPI_SLAVE) {
    SPI_DDR &= ~(1<<SPI_SS_PIN);    // input
    SPI_DDR &= ~(1<<SPI_SCK_PIN);   // input
    SPI_DDR &= ~(1<<SPI_MOSI_PIN);  // input
    SPI_DDR |= (1<<SPI_MISO_PIN);   // output
    SPCR &= ~(1 << MSTR); // set to slave mode
  }
#endif
#ifdef EASYRIDER_MCU2
  if (role == SPI_OFF) { // SPI_OFF for mcu2
    // restore hardware SS pin's current role (sense for LI Left Indicator)
    SPI_DDR &= ~(1<<SPI_SS_PIN); // input
    SPI_PORT |= (1<<SPI_SS_PIN); // high
    SPI_SLAVE_DDR |= (1<<SPI_SDSLAVE_PIN);   // output
    SPI_SLAVE_DDR |= (1<<SPI_MCUSLAVE_PIN);  // output
  } else if (role == SPI_MASTER) {
    SPI_DDR |= (1<<SPI_SCK_PIN);    // output
    SPI_DDR |= (1<<SPI_MOSI_PIN);   // output
    SPI_DDR &= ~(1<<SPI_MISO_PIN);  // input
    // disable hardware SS pin's current role (sense for LI Left Indicator)
    SPI_DDR |= (1<<SPI_SS_PIN);     // output
    SPI_PORT &= ~(1<<SPI_SS_PIN);   // low, double toggle seems useful, read that somewhere
    SPI_PORT |= (1<<SPI_SS_PIN);    // high
    SPI_SLAVE_DDR |= (1<<SPI_SDSLAVE_PIN);   // output
    SPI_SLAVE_DDR |= (1<<SPI_MCUSLAVE_PIN);  // output
    SPCR |= (1 << MSTR); // set to master mode
  }
#endif
  spi_setdatamode(mode);
  spi_setbitorder(bitorder);
  spi_setclockdivider(speed);
  if (interrupt) {
    spi_attachinterrupt();
  }
}

// enable SPI
void spi_enable() {
  SPCR |= (1 << SPE);
}

// disable all SPI operations
void spi_disable() {
  SPCR = 0;
}

// start intercommunication between mcu's
void spi_mcu_start() {
  SPI_SLAVE_PORT &= ~(1<<SPI_MCUSLAVE_PIN);
}

// stop intercommunication between mcu's
void spi_mcu_stop() {
  SPI_SLAVE_PORT |= (1<<SPI_MCUSLAVE_PIN);
}

// start SD card communication
void spi_sd_start() {
  SPI_SLAVE_PORT &= ~(1<<SPI_SDSLAVE_PIN);
}

// stop SD card communication
void spi_sd_stop() {
  SPI_SLAVE_PORT |= (1<<SPI_SDSLAVE_PIN);
}

// set the byte to be transferred
void spi_set(uint8_t data) {
  SPDR = data; // transmit the byte to be sent
}

// return receiving byte
uint8_t spi_get() {
  return SPDR;
}

// sending/receiving the bytes
// blocking version (non-interrupt)
uint8_t spi_communicate(uint8_t data) {
  SPDR = data; // transmit the byte to be sent
  while (!(SPSR & (1 << SPIF))); // wait for the transfer to complete
  return SPDR; // return the byte the slave just returned (double buffered)
}

// set SPI data order (lsb or msb first)
void spi_setbitorder(uint8_t bitorder) {
  if (bitorder == SPI_LSBFIRST) {
    SPCR |= (1 << DORD);
  } else {
    SPCR &= ~(1 << DORD);
  }
}

// set SPI mode (rising/falling sample/setup)
void spi_setdatamode(uint8_t mode) {
  SPCR = (SPCR & ~SPI_MODE_MASK) | mode; // clear 2nd/3rd bit and set new mode bits
}

// set SPI clock speed
void spi_setclockdivider(uint8_t rate) {
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK); // clear the last 2 lsb bits and set them to the rate constant
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK); // clear the last lsb bit and set new SPI2X bit by shifting rate 2 places to the right
}

// enable interrupt flag
void spi_attachinterrupt() {
  SPCR |= (1 << SPIE);
}

