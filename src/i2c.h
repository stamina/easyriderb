/*
 *
 *  Copyright (C) Bas Brugman
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
 *  Based on Peter Fleury's library
 *  http://homepage.hispeed.ch/peterfleury/index.html
 *
 */
#ifndef I2C_H_INCLUDED
#define I2C_H_INCLUDED

#include <avr/io.h>

#define I2C_READ    1
#define I2C_WRITE   0

/**
 @brief initialize the I2C master interace. Need to be called only once 
 @param  void
 @return none
 */
void i2c_init(void);

/** 
 @brief Terminates the data transfer and releases the I2C bus 
 @param void
 @return none
 */
void i2c_stop(void);

/** 
 @brief Issues a start condition and sends address and transfer direction 
 @param    addr address and transfer direction of I2C device
 @retval   0   device accessible 
 @retval   1   failed to access device 
 */
unsigned char i2c_start(unsigned char addr);

/**
 @brief Issues a repeated start condition and sends address and transfer direction 
 @param   addr address and transfer direction of I2C device
 @retval  0 device accessible
 @retval  1 failed to access device
 */
unsigned char i2c_rep_start(unsigned char addr);

/**
 @brief Issues a start condition and sends address and transfer direction 
 If device is busy, use ack polling to wait until device ready 
 @param    addr address and transfer direction of I2C device
 @return   none
 */
void i2c_start_wait(unsigned char addr);
 
/**
 @brief Send one byte to I2C device
 @param    data  byte to be transfered
 @retval   0 write successful
 @retval   1 write failed
 */
unsigned char i2c_write(unsigned char data);


/**
 @brief    read one byte from the I2C device, request more data from device 
 @return   byte read from I2C device
 */
unsigned char i2c_readAck(void);

/**
 @brief    read one byte from the I2C device, read is followed by a stop condition 
 @return   byte read from I2C device
 */
unsigned char i2c_readNak(void);

/** 
 @brief    read one byte from the I2C device
 Implemented as a macro, which calls either i2c_readAck or i2c_readNak
 @param    ack 1 send ack, request more data from device<br>
               0 send nak, read is followed by a stop condition 
 @return   byte read from I2C device
 */
unsigned char i2c_read(unsigned char ack);
#define i2c_read(ack)  (ack) ? i2c_readAck() : i2c_readNak(); 

/**@}*/
#endif
