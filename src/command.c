/* license{{{
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
 * }}}
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <util/delay.h>  // used for tiny halts, e.g. to have the SPI
                         // slave preparation time before the master sends

#include "command.h"

// command queue size
#define CMD_BUFFER_SIZE 255
// max size of a single command
#define CMD_PAYLOAD_SIZE 125
// size of control bytes in a command
#define CMD_CONTROL_SIZE 3

// payload sizes per command
#ifdef EASYRIDER_MCU1
// example: 0x01 - b - 3403701080250119954214000000 - 0x02
#define CMD_STATS_SIZE (28 + CMD_CONTROL_SIZE)
#endif

#ifdef EASYRIDER_MCU2
// example: 0x01 - a - Dddmmyyhhmmssmmm00000000.00000001 - 0x02
#define CMD_STATE_SIZE (17 + CMD_CONTROL_SIZE + TS_SIZE)
// example: 0x01 - b - 0x02
#define CMD_STATS_SIZE CMD_CONTROL_SIZE
#endif

// example: 0x01 - c - Dddmmyyhhmmssmmm3403701080250119954214000000 - 0x02
#define CMD_DATA_SIZE (28 + CMD_CONTROL_SIZE + TS_SIZE)
// example: 0x01 - d - Dddmmyyhhmmssmmm
#define CMD_GPS_SIZE (75 + CMD_CONTROL_SIZE + TS_SIZE)
// example: 0x01 -
#define CMD_RTC_SIZE 0
// example: 0x01 -
#define CMD_SENSE_SIZE 0
// example: 0x01 -
#define CMD_SETTINGS_SIZE 0
// example: 0x01 -
#define CMD_LOG_SIZE 0
// example: 0x01 -
#define CMD_REBOOT_SIZE 0
// example: 0x01 -
#define CMD_PINCODE_SIZE 0
// example: 0x01 - k - 255 - 0x02
#define CMD_SOUND_SIZE (3 + CMD_CONTROL_SIZE)
// example: 0x01 -
#define CMD_MSG_SIZE 0

/* proto1{{{*/

char g_state_byte[10];  // binary string of low/high state bytes
static char g_in_payload[CMD_PAYLOAD_SIZE];  // incoming command bytes without
                                             // start/stop bytes: [CMD] byte
                                             // followed by [1..n] data bytes
                                             // SPI internal communication

static char g_out_payload[CMD_PAYLOAD_SIZE];  // outgoing combined bytes of the
                                              // command payload, excluding
                                              // start/cmd ID/stop bytes
                                              // SPI internal communication

#ifdef EASYRIDER_MCU1
static char g_ext_payload[CMD_PAYLOAD_SIZE];  // incoming command bytes without
                                              // start/stop bytes: [CMD] byte
                                              // followed by [1..n] data bytes
                                              // serial wifi/FTDI uart external
                                              // communication
#endif

static volatile uint8_t g_in_byte;   // latest incoming byte
static volatile uint8_t g_out_byte;  // latest outgoing byte
static volatile uint8_t
    g_mcu_in_cmds[CMD_BUFFER_SIZE];  // mcu intercommunication incoming commands
static volatile int8_t g_mcu_out_cmds[CMD_BUFFER_SIZE];  // mcu
                                                         // intercommunication
                                                         // outcoming commands
static volatile uint8_t g_mcu_in_tail;
static volatile uint8_t g_mcu_in_head;
static volatile uint8_t g_mcu_out_tail;
static volatile uint8_t g_mcu_out_head;

static uint8_t get_mcu_in_byte(void);
static void set_mcu_in_byte(uint8_t bt);
static uint8_t get_mcu_out_byte(void);
static void set_mcu_out_byte(uint8_t bt);
static uint8_t mcu_in_available(void);
static uint8_t mcu_out_available(void);
static void command_launch(uint8_t cmd, tCMDInterface cmd_interface);

// all single char commands, used for validation
static const char* g_all_cmds = ALL_CMDS;

static volatile t_cmd_status g_in_status = IDLE;   // status of incoming command
static volatile t_cmd_status g_out_status = IDLE;  // status of outgoing command
/*}}}*/
#ifdef EASYRIDER_MCU2
extern volatile uint16_t g_state;  // current state
// timestamp size
#define TS_SIZE 17
static char g_timestamp[TS_SIZE];  // [0-15], 16 ascii chars ->
                                   // [Dddmmyyhhmmssmmm] + NULL
extern RTCDate g_datetime;         // date/time from RTC
static void send_cmd(void);
#endif
#ifdef EASYRIDER_MCU1
extern void set_sound(uint8_t status);
extern volatile uint16_t g_current;      // board current consumption in mA
extern volatile uint16_t g_rpm;          // current RPM of engine
extern volatile uint16_t g_voltage;      // battery voltage
extern volatile uint16_t g_temperature;  // board ambient temperature
static void command_data_handler(void);
static void command_state_handler(void);
static void command_gps_handler(void);
static void command_sound_handler(void);
#endif
static void command_stats_handler(void);
extern volatile uint16_t g_accelx;  // X-axis voltage of accelerometer
extern volatile uint16_t g_accely;  // Y-axis voltage of accelerometer
extern volatile uint16_t g_accelz;  // Z-axis voltage of accelerometer
extern volatile uint8_t g_gear;     // current gear
/*main command process functions{{{*/

uint8_t get_mcu_in_byte() {
  uint8_t bt;
  if (g_mcu_in_tail != g_mcu_in_head) {  // not empty
    bt = g_mcu_in_cmds[g_mcu_in_tail];
    g_mcu_in_tail++;
    if (g_mcu_in_tail >= CMD_BUFFER_SIZE) {  // tail to front
      g_mcu_in_tail = 0;
    }
    return bt;
  } else {
    return CMD_NOOP;  // no cmd bytes, return No Operation byte
  }
}

void set_mcu_in_byte(uint8_t bt) {
  g_mcu_in_cmds[g_mcu_in_head] = bt;  // insert byte at head position
  g_mcu_in_head++;                    // advance head
  if (g_mcu_in_head >= CMD_BUFFER_SIZE) {
    g_mcu_in_head = 0;  // cycle back to start
  }
  if (g_mcu_in_head == g_mcu_in_tail) {
    g_mcu_in_tail++;  // also move tail, basically destroying the oldest event
                      // to make space
  }
  if (g_mcu_in_tail >= CMD_BUFFER_SIZE) {
    g_mcu_in_tail = 0;  // cycle back to start
  }
}

uint8_t get_mcu_out_byte() {
  uint8_t bt;
  if (g_mcu_out_tail != g_mcu_out_head) {  // not empty
    bt = g_mcu_out_cmds[g_mcu_out_tail];
    g_mcu_out_tail++;
    if (g_mcu_out_tail >= CMD_BUFFER_SIZE) {  // tail to front
      g_mcu_out_tail = 0;
    }
    return bt;
  } else {
    return CMD_NOOP;  // no cmd bytes, return No Operation byte
  }
}

void set_mcu_out_byte(uint8_t bt) {
  g_mcu_out_cmds[g_mcu_out_head] = bt;  // insert byte at head position
  g_mcu_out_head++;                     // advance head
  if (g_mcu_out_head >= CMD_BUFFER_SIZE) {
    g_mcu_out_head = 0;  // cycle back to start
  }
  if (g_mcu_out_head == g_mcu_out_tail) {
    // NOTE: this buffer overwriting should never happen since we pre-check
    // space
    g_mcu_out_tail++;  // also move tail, basically destroying the oldest event
                       // to make space
  }
  if (g_mcu_out_tail >= CMD_BUFFER_SIZE) {
    g_mcu_out_tail = 0;  // cycle back to start
  }
}

// return the number of bytes waiting in incoming buffer
uint8_t mcu_in_available() {
  uint8_t head, tail;
  head = g_mcu_in_head;
  tail = g_mcu_in_tail;
  if (head >= tail) return head - tail;  // return count of bytes inbetween
  return CMD_BUFFER_SIZE + head - tail;  // head has rolled over to start,
                                         // return count of bytes inbetween
}

// return the number of bytes waiting in outgoing buffer
uint8_t mcu_out_available() {
  uint8_t head, tail;
  head = g_mcu_out_head;
  tail = g_mcu_out_tail;
  if (head >= tail) return head - tail;  // return count of bytes inbetween
  return CMD_BUFFER_SIZE + head - tail;  // head has rolled over to start,
                                         // return count of bytes inbetween
}

// initialize all interface communication protocols
void command_init() {
  uart_init_0();
  uart_init_1();
#ifdef EASY_TRACE
  uart_put_str_1("CMD_INIT\r\n");
#endif
#ifdef EASYRIDER_MCU1
  g_spi_state = SPI_SLAVE;
  spi_init(g_spi_state, SPI_CLOCK_DIV4, SPI_MODE0, SPI_MSBFIRST, SPI_INTERRUPT);
  spi_enable();
  spi_set(CMD_NOOP);  // set initial slave byte, no operation
  // wifi init
  wifi_init();
#endif
#ifdef EASYRIDER_MCU2
  _delay_ms(
      1);  // short delay to give the slave mcu enough time to setup his SPI
  g_spi_state = SPI_OFF;  // init to SPI off
  spi_init(g_spi_state, SPI_CLOCK_DIV4, SPI_MODE0, SPI_MSBFIRST,
           SPI_NO_INTERRUPT);
  ds1307_init();  // RTC
#endif
}

#ifdef EASYRIDER_MCU1
// SPI byte received interrupt
// get newly shifted inbyte and set newly outbyte
ISR(SPI_STC_vect) {
  g_in_byte = spi_get();
  if (g_in_byte != CMD_NOOP) {  // don't save No Op bytes
    set_mcu_in_byte(g_in_byte);
  }
  g_out_byte = get_mcu_out_byte();
  spi_set(g_out_byte);
}
#endif

#ifdef EASYRIDER_MCU2
// transceive SPI polling function: send/receive a byte from the queue
void send_cmd() {
  g_out_byte = get_mcu_out_byte();
  g_in_byte = spi_communicate(g_out_byte);
  if (g_in_byte != CMD_NOOP) {
    set_mcu_in_byte(g_in_byte);
  }
  _delay_us(5);
}
#endif

// process commands to be transmitted
// also toggle SPI off when there's no communication needed
void command_process() {
  static uint8_t x;  // index for g_in_payload
  uint8_t in_byte;
#ifdef EASYRIDER_MCU2
  // out buffer is empty, go to IDLE state
  if (!mcu_out_available()) g_out_status = IDLE;
  // outgoing data available or slave is still sending a command byte
  if ((mcu_out_available()) || (g_in_byte != CMD_NOOP)) {
    if (g_spi_state == SPI_OFF) {  // start SPI as master
      g_spi_state = SPI_MASTER;
      spi_init(g_spi_state, SPI_CLOCK_DIV4, SPI_MODE0, SPI_MSBFIRST,
               SPI_NO_INTERRUPT);
      spi_enable();
      spi_mcu_start();
    }
    send_cmd();                         // send 1 byte at a time
  } else if (g_spi_state != SPI_OFF) {  // disable SPI
    spi_mcu_stop();
    spi_disable();
    g_spi_state = SPI_OFF;
    spi_init(g_spi_state, SPI_CLOCK_DIV4, SPI_MODE0, SPI_MSBFIRST,
             SPI_NO_INTERRUPT);
  }
#endif
  // in bytes available
  if (mcu_in_available()) {
    in_byte = get_mcu_in_byte();
    switch (g_in_status) {
      case IDLE:
        if (in_byte == CMD_START) {
          g_in_status = START;
        }
        break;
      case START:
        if (strchr(g_all_cmds, in_byte) != NULL) {  // check for valid command
          g_in_status = CMD;
          x = 0;
          g_in_payload[x] =
              in_byte;  // command byte is always the first char of payload
          x++;
        } else {
          g_in_status = IDLE;  // reset, no CMD found after START
        }
        break;
      case CMD:
        if (in_byte == CMD_STOP) {  // stop byte found
          g_in_payload[x] = '\0';
          command_launch(g_in_payload[0], CMD_IF_IC);  // process command now
          g_in_status = IDLE;                          // reset
        } else if (in_byte == CMD_START) {  // found a re-start -> ignore
                                            // current incomplete cmd
          g_in_status = START;
        } else if (x < CMD_PAYLOAD_SIZE - 1) {  // fill payload
          g_in_payload[x] = in_byte;
          x++;
        } else {
          g_in_status = IDLE;  // reset, buffer overflowed
        }
        break;
      default:
        g_in_status = IDLE;  // reset, undefined state
    }
  }
#ifdef EASYRIDER_MCU1
  // process wifi messages
  wifi_process();
  // serial_process();
#endif
#ifdef EASYRIDER_MCU2
  // process gps messages
  gps_process();
#endif
}
/*}}}*/
#ifdef EASYRIDER_MCU1
// dispatches the incoming wifi/serial commands to their handlers
void command_dispatch(const char* data) {
  strncpy(g_ext_payload, data, CMD_PAYLOAD_SIZE - 1);
  command_launch(g_ext_payload[0], CMD_IF_EXT);
#ifdef EASY_TRACE
  uart_put_str_1("CMD_DISPATCH");
#endif
}

// dispatches the incoming SPI commands to their handlers
void command_launch(uint8_t cmd, tCMDInterface cmd_interface) {
  switch (cmd) {
    case CMD_STATE:
      command_state_handler();
      break;
    case CMD_STATS:
      command_stats_handler();
      break;
    case CMD_DATA:
      command_data_handler();
      break;
    case CMD_GPS:
      command_gps_handler();
      break;
    case CMD_SOUND:
      command_sound_handler();
      break;
    default:;
      ;
#ifdef EASY_TRACE
      uart_put_str_1("IN_CMD_UNKNOWN\r\n");
#endif
  }
}
#endif

#ifdef EASYRIDER_MCU2
// dispatches the incoming SPI commands to their handlers
void command_launch(uint8_t cmd, tCMDInterface cmd_interface) {
  switch (cmd) {
    case CMD_STATS:
      command_stats_handler();
      break;
    default:;
      ;
#ifdef EASY_TRACE
      uart_put_str_1("IN_CMD_UNKNOWN\r\n");
#endif
  }
}
#endif

#ifdef EASYRIDER_MCU2
// onchange trigger -> has prio
uint8_t command_trigger_state(tCMDInterface cmd_interface) {
  if (cmd_interface == CMD_IF_IC) {  // mcu intercommunication
    if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_STATE_SIZE) {
      g_out_status = BUSY;
      g_out_payload[0] = '\0';
      g_state_byte[0] = '\0';
      command_util_get_timestamp();        // update timestamp
      strcat(g_out_payload, g_timestamp);  // ts
      strcat(g_out_payload,
             command_util_btob(g_state >> 8, g_state_byte));  // high state byte
      strcat(g_out_payload, ".");
      g_state_byte[0] = '\0';
      strcat(g_out_payload, command_util_btob((uint8_t)g_state,
                                              g_state_byte));  // low state byte
      set_mcu_out_byte(CMD_START);
      set_mcu_out_byte(CMD_STATE);
      char* ptr = g_out_payload;
      while (*ptr) {
        set_mcu_out_byte(*ptr);
        ptr++;
      }
      set_mcu_out_byte(CMD_STOP);
    } else {
      return 0;
    }
  } else if (cmd_interface == CMD_IF_DEBUG) {  // debug over uart1
    g_state_byte[0] = '\0';
    uart_put_str_1("TRACE_STATE\r\n");
    uart_put_str_1(command_util_btob(g_state >> 8, g_state_byte));
    g_state_byte[0] = '\0';
    uart_put_str_1(".");
    uart_put_str_1(command_util_btob((uint8_t)g_state, g_state_byte));
    uart_put_str_1("\r\n");
  }
  return 1;
}

// periodic trigger
uint8_t command_trigger_gps(tCMDInterface cmd_interface) {
  if (cmd_interface == CMD_IF_IC) {  // mcu intercommunication
    if (g_out_status == BUSY) return 0;
    if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_GPS_SIZE) {
      g_out_status = BUSY;
      g_out_payload[0] = '\0';
      command_util_get_timestamp();        // update timestamp
      strcat(g_out_payload, g_timestamp);  // ts
      if (gps_status) {  // the latest valid GPS string created
        strcat(g_out_payload, gps_ascii);
      }
      set_mcu_out_byte(CMD_START);
      set_mcu_out_byte(CMD_GPS);
      char* ptr = g_out_payload;
      while (*ptr) {
        set_mcu_out_byte(*ptr);
        ptr++;
      }
      set_mcu_out_byte(CMD_STOP);
    } else {
      return 0;
    }
  } else if (cmd_interface == CMD_IF_DEBUG) {  // debug over uart1
    uart_put_str_1("TRACE_GPS\r\n");
  }
  return 1;
}

// periodic trigger
uint8_t command_trigger_stats(tCMDInterface cmd_interface) {
  if (cmd_interface == CMD_IF_IC) {  // mcu intercommunication
    if (g_out_status == BUSY) return 0;
    if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_STATS_SIZE) {
      g_out_status = BUSY;
      set_mcu_out_byte(CMD_START);
      set_mcu_out_byte(CMD_STATS);
      set_mcu_out_byte(CMD_STOP);
    } else {
      return 0;
    }
  } else if (cmd_interface == CMD_IF_DEBUG) {  // debug over uart1
    uart_put_str_1("TRACE_STATS\r\n");
  }
  return 1;
}

// give sound command to MCU1's buzzer
uint8_t command_trigger_sound(uint8_t status, tCMDInterface cmd_interface) {
  if (cmd_interface == CMD_IF_IC) {  // mcu intercommunication
    if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_SOUND_SIZE) {
      g_out_status = BUSY;
      char tmp_sound_status[4];
      memset((void*)tmp_sound_status, 0, 4);
      fixed_ascii_uint8(tmp_sound_status, status);
#ifdef EASY_TRACE
      uart_put_str_1("SOUND_CMD: ");
      uart_put_str_1(tmp_sound_status);
      uart_put_str_1("\r\n");
#endif
      set_mcu_out_byte(CMD_START);
      set_mcu_out_byte(CMD_SOUND);
      char* ptr = tmp_sound_status;
      while (*ptr) {
        set_mcu_out_byte(*ptr);
        ptr++;
      }
      set_mcu_out_byte(CMD_STOP);
    } else {
      return 0;
    }
  } else if (cmd_interface == CMD_IF_DEBUG) {  // debug over uart1
    uart_put_str_1("TRACE_SOUND\r\n");
  }
  return 1;
}
#endif

#ifdef EASYRIDER_MCU1
// creates the ascii representation of the sensor statistics
// 28 chars: [xxxyyyzzzCCCCVVVVVTTTTRRRRRG]
// idx:      [0123456789012345678901234567]
// xyz accelerometer, current, voltage, temperature, rpm and gear
void command_stats_handler() {
  char tmp_stat[6];
  g_out_payload[0] = '\0';
  memset((void*)g_out_payload, 0x30,
         CMD_PAYLOAD_SIZE);  // prefill with ascii 0's
  // xxx x-accelerometer
  utoa(g_accelx, tmp_stat, 10);
  if (g_accelx < 10) {
    g_out_payload[2] = tmp_stat[0];
  } else if (g_accelx < 100) {
    g_out_payload[1] = tmp_stat[0];
    g_out_payload[2] = tmp_stat[1];
  } else {
    g_out_payload[0] = tmp_stat[0];
    g_out_payload[1] = tmp_stat[1];
    g_out_payload[2] = tmp_stat[2];
  }
  // yyy x-accelerometer
  utoa(g_accely, tmp_stat, 10);
  if (g_accely < 10) {
    g_out_payload[5] = tmp_stat[0];
  } else if (g_accely < 100) {
    g_out_payload[3] = tmp_stat[0];
    g_out_payload[4] = tmp_stat[1];
  } else {
    g_out_payload[3] = tmp_stat[0];
    g_out_payload[4] = tmp_stat[1];
    g_out_payload[5] = tmp_stat[2];
  }
  // zzz x-accelerometer
  utoa(g_accelz, tmp_stat, 10);
  if (g_accelz < 10) {
    g_out_payload[8] = tmp_stat[0];
  } else if (g_accelz < 100) {
    g_out_payload[6] = tmp_stat[0];
    g_out_payload[7] = tmp_stat[1];
  } else {
    g_out_payload[6] = tmp_stat[0];
    g_out_payload[7] = tmp_stat[1];
    g_out_payload[8] = tmp_stat[2];
  }
  // CCCC current consumption in mA
  utoa(g_current, tmp_stat, 10);
  if (g_current < 10) {
    g_out_payload[12] = tmp_stat[0];
  } else if (g_current < 100) {
    g_out_payload[11] = tmp_stat[0];
    g_out_payload[12] = tmp_stat[1];
  } else if (g_current < 1000) {
    g_out_payload[10] = tmp_stat[0];
    g_out_payload[11] = tmp_stat[1];
    g_out_payload[12] = tmp_stat[2];
  } else {
    g_out_payload[9] = tmp_stat[0];
    g_out_payload[10] = tmp_stat[1];
    g_out_payload[11] = tmp_stat[2];
    g_out_payload[12] = tmp_stat[3];
  }
  // VVVVV voltage in mV
  utoa(g_voltage, tmp_stat, 10);
  if (g_voltage < 10) {
    g_out_payload[17] = tmp_stat[0];
  } else if (g_voltage < 100) {
    g_out_payload[16] = tmp_stat[0];
    g_out_payload[17] = tmp_stat[1];
  } else if (g_voltage < 1000) {
    g_out_payload[15] = tmp_stat[0];
    g_out_payload[16] = tmp_stat[1];
    g_out_payload[17] = tmp_stat[2];
  } else if (g_voltage < 10000) {
    g_out_payload[14] = tmp_stat[0];
    g_out_payload[15] = tmp_stat[1];
    g_out_payload[16] = tmp_stat[2];
    g_out_payload[17] = tmp_stat[3];
  } else {
    g_out_payload[13] = tmp_stat[0];
    g_out_payload[14] = tmp_stat[1];
    g_out_payload[15] = tmp_stat[2];
    g_out_payload[16] = tmp_stat[3];
    g_out_payload[17] = tmp_stat[4];
  }
  // TTTT temperature in mCelsius
  utoa(g_temperature, tmp_stat, 10);
  if (g_temperature < 10) {
    g_out_payload[21] = tmp_stat[0];
  } else if (g_temperature < 100) {
    g_out_payload[20] = tmp_stat[0];
    g_out_payload[21] = tmp_stat[1];
  } else if (g_temperature < 1000) {
    g_out_payload[19] = tmp_stat[0];
    g_out_payload[20] = tmp_stat[1];
    g_out_payload[21] = tmp_stat[2];
  } else {
    g_out_payload[18] = tmp_stat[0];
    g_out_payload[19] = tmp_stat[1];
    g_out_payload[20] = tmp_stat[2];
    g_out_payload[21] = tmp_stat[3];
  }
  // RRRRR rpm
  utoa(g_rpm, tmp_stat, 10);
  if (g_rpm < 10) {
    g_out_payload[26] = tmp_stat[0];
  } else if (g_rpm < 100) {
    g_out_payload[25] = tmp_stat[0];
    g_out_payload[26] = tmp_stat[1];
  } else if (g_rpm < 1000) {
    g_out_payload[24] = tmp_stat[0];
    g_out_payload[25] = tmp_stat[1];
    g_out_payload[26] = tmp_stat[2];
  } else if (g_rpm < 10000) {
    g_out_payload[23] = tmp_stat[0];
    g_out_payload[24] = tmp_stat[1];
    g_out_payload[25] = tmp_stat[2];
    g_out_payload[26] = tmp_stat[3];
  } else {
    g_out_payload[22] = tmp_stat[0];
    g_out_payload[23] = tmp_stat[1];
    g_out_payload[24] = tmp_stat[2];
    g_out_payload[25] = tmp_stat[3];
    g_out_payload[26] = tmp_stat[4];
  }
  // G gear selection
  utoa(g_gear, tmp_stat, 10);
  g_out_payload[27] = tmp_stat[0];
  g_out_payload[28] = '\0';
  // CMD_STATS command
  if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_STATS_SIZE) {
    char* ptr = g_out_payload;
    set_mcu_out_byte(CMD_START);
    set_mcu_out_byte(CMD_STATS);
    while (*ptr) {
      set_mcu_out_byte(*ptr);
      ptr++;
    }
    set_mcu_out_byte(CMD_STOP);
  }
}

// incoming data from mcu2, passed directly to wifi
void command_data_handler() {
  wifi_dispatch(g_in_payload);
#ifdef EASY_TRACE
  uart_put_str_1("WIFI_DISPATCH CMD_DATA: ");
  uart_put_str_1(g_in_payload);
  uart_put_str_1("\r\n");
#endif
}

// incoming data from mcu2, passed directly to wifi
void command_state_handler() {
  wifi_dispatch(g_in_payload);
#ifdef EASY_TRACE
  uart_put_str_1("WIFI_DISPATCH CMD_STATE: ");
  uart_put_str_1(g_in_payload);
  uart_put_str_1("\r\n");
#endif
}

// incoming data from mcu2, passed directly to wifi
void command_gps_handler() {
  wifi_dispatch(g_in_payload);
#ifdef EASY_TRACE
  uart_put_str_1("WIFI_DISPATCH CMD_GPS: ");
  uart_put_str_1(g_in_payload);
  uart_put_str_1("\r\n");
#endif
}

// incoming data from mcu2, trigger a sound
void command_sound_handler() {
  char status[4];
  strlcpy(status, &g_in_payload[1], 4);
  set_sound(atoi(status));  // status 0-255
#ifdef EASY_TRACE
  uart_put_str_1("DISPATCH CMD_SOUND: ");
  uart_put_str_1(&g_in_payload[1]);
  uart_put_str_1("\r\n");
#endif
}
#endif

#ifdef EASYRIDER_MCU2
// process incoming stats, apply a timestamp
// and send it back as a CMD_DATA command
void command_stats_handler() {
  if ((CMD_BUFFER_SIZE - mcu_out_available()) >= CMD_DATA_SIZE) {
    g_out_payload[0] = '\0';
    command_util_get_timestamp();        // update timestamp
    strcat(g_out_payload, g_timestamp);  // ts
    strcat(g_out_payload,
           &g_in_payload[1]);  // all stats without command byte
    char* ptr = g_out_payload;
    set_mcu_out_byte(CMD_START);
    set_mcu_out_byte(CMD_DATA);
    while (*ptr) {
      set_mcu_out_byte(*ptr);
      ptr++;
    }
    set_mcu_out_byte(CMD_STOP);
  }
  // save some MCU1 stats for further checks
  char l_gear[2];
  char l_accelx[4];
  char l_accely[4];
  char l_accelz[4];
  strlcpy(l_gear, &g_in_payload[28], 2);
  strlcpy(l_accelx, &g_in_payload[1], 4);
  strlcpy(l_accely, &g_in_payload[4], 4);
  strlcpy(l_accelz, &g_in_payload[7], 4);
  g_gear = atoi(l_gear);
  g_accelx = atoi(l_accelx);
  g_accely = atoi(l_accely);
  g_accelz = atoi(l_accelz);
#ifdef EASY_TRACE
  uart_put_str_1("g_gear: ");
  uart_put_int_1(g_gear);
  uart_put_str_1("\r\n");
  uart_put_str_1("x: ");
  uart_put_int_1(g_accelx);
  uart_put_str_1("\r\n");
  uart_put_str_1("y: ");
  uart_put_int_1(g_accely);
  uart_put_str_1("\r\n");
  uart_put_str_1("z: ");
  uart_put_int_1(g_accelz);
  uart_put_str_1("\r\n");
#endif
}

/*command_util_get_timestamp(){{{*/
// creates the ascii representation of the timestamp
// 16 parts: [Dddmmyyhhmmssmmm]
// idx:      [0123456789012345]
void command_util_get_timestamp() {
  char tmp_ts[4];
  memset((void*)g_timestamp, 0x30, TS_SIZE);  // prefill with 0's
  // weekday 1-7
  utoa(g_datetime.weekday, tmp_ts, 10);
  g_timestamp[0] = tmp_ts[0];
  // day of month 1-31
  utoa(g_datetime.day, tmp_ts, 10);
  if (g_datetime.day > 9) {
    g_timestamp[1] = tmp_ts[0];
    g_timestamp[2] = tmp_ts[1];
  } else {
    g_timestamp[2] = tmp_ts[0];
  }
  // month 1-12
  utoa(g_datetime.month, tmp_ts, 10);
  if (g_datetime.month > 9) {
    g_timestamp[3] = tmp_ts[0];
    g_timestamp[4] = tmp_ts[1];
  } else {
    g_timestamp[4] = tmp_ts[0];
  }
  // year 0-99
  utoa(g_datetime.year, tmp_ts, 10);
  if (g_datetime.year > 9) {
    g_timestamp[5] = tmp_ts[0];
    g_timestamp[6] = tmp_ts[1];
  } else {
    g_timestamp[6] = tmp_ts[0];
  }
  // hours 0-23
  utoa(g_datetime.hours, tmp_ts, 10);
  if (g_datetime.hours > 9) {
    g_timestamp[7] = tmp_ts[0];
    g_timestamp[8] = tmp_ts[1];
  } else {
    g_timestamp[8] = tmp_ts[0];
  }
  // mins 0-59
  utoa(g_datetime.minutes, tmp_ts, 10);
  if (g_datetime.minutes > 9) {
    g_timestamp[9] = tmp_ts[0];
    g_timestamp[10] = tmp_ts[1];
  } else {
    g_timestamp[10] = tmp_ts[0];
  }
  // secs 0-59
  utoa(g_datetime.seconds, tmp_ts, 10);
  if (g_datetime.seconds > 9) {
    g_timestamp[11] = tmp_ts[0];
    g_timestamp[12] = tmp_ts[1];
  } else {
    g_timestamp[12] = tmp_ts[0];
  }
  // ms 0-999
  utoa(g_datetime.milliseconds, tmp_ts, 10);
  if (g_datetime.milliseconds > 99) {
    g_timestamp[13] = tmp_ts[0];
    g_timestamp[14] = tmp_ts[1];
    g_timestamp[15] = tmp_ts[2];
  } else if (g_datetime.milliseconds > 9) {
    g_timestamp[14] = tmp_ts[0];
    g_timestamp[15] = tmp_ts[1];
  } else {
    g_timestamp[15] = tmp_ts[0];
  }
  // terminate
  g_timestamp[TS_SIZE - 1] = '\0';
} /*}}}*/
#endif
