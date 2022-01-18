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
#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>


#define SPI_SS_PIN            PB4 
#define SPI_MOSI_PIN          PB5
#define SPI_MISO_PIN          PB6
#define SPI_SCK_PIN           PB7
#define SPI_DDR               DDRB
#define SPI_PORT              PORTB

// MCU2 only
#define SPI_SDSLAVE_PIN       PA1
#define SPI_MCUSLAVE_PIN      PA2 
#define SPI_SLAVE_DDR         DDRA
#define SPI_SLAVE_PORT        PORTA

#define SPI_LSBFIRST 0 // shift out least significant bit first
#define SPI_MSBFIRST 1 // shift out most significant bit first

// raise interrupt when byte received (SPIF bit)
#define SPI_NO_INTERRUPT 0
#define SPI_INTERRUPT 1

// SPR [1:0] of SPCR  and SPI2X bit of SPSR register decides frequency of SCK. The combination of these three bits will be used to select SCK frequency
#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2_2X 0x04 // SPI2x comes into play now
#define SPI_CLOCK_DIV8_2X 0x05
#define SPI_CLOCK_DIV32_2X 0x06
#define SPI_CLOCK_DIV64_2X 0x07

// SPI modes (rising/falling sample/setup)
#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

typedef enum {
  SPI_OFF, // only for mcu2 to restore the SS pin for SI_LI usage again
  SPI_SLAVE, // permanent state for mcu1
  SPI_MASTER // mcu2 goes into master mode when commands are available
} tSPIState;

tSPIState g_spi_state;

void spi_init(tSPIState role, uint8_t speed,  uint8_t mode, uint8_t bitorder, uint8_t interrupt);
void spi_enable(void);
void spi_disable(void);
void spi_mcu_start(void);
void spi_mcu_stop(void);
void spi_sd_start(void);
void spi_sd_stop(void);
void spi_set(uint8_t data);
uint8_t spi_get(void);
uint8_t spi_communicate(uint8_t data);
void spi_setbitorder(uint8_t bitorder);
void spi_setdatamode(uint8_t mode);
void spi_setclockdivider(uint8_t rate);
void spi_attachinterrupt(void);

#endif

