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
#include "wifi.h"

// max size of a single command
#define CMD_PAYLOAD_SIZE 125

static const char *g_all_cmds = ALL_CMDS;
static volatile t_cmd_status g_in_status = IDLE;  // status of incoming command
static char g_in_payload[CMD_PAYLOAD_SIZE];  // command bytes without start/stop
                                             // bytes: [CMD] byte followed by
                                             // [1..n] data bytes
// usart function pointers
static void (*wifi_send)(const char *str);
static void (*wifi_send_byte)(uint8_t c);
static uint8_t (*wifi_get_byte)(void);
static uint8_t (*wifi_available)(void);
static void (*wifi_flush)(void);

void wifi_init(void) {
  wifi_send = uart_put_str_0;
  wifi_send_byte = uart_put_0;
  wifi_get_byte = uart_get_0;
  wifi_available = uart_available_0;
  wifi_flush = uart_flush_rx_0;
}

// function called from command.c repetitively, handles all incoming wifi data
void wifi_process(void) {
  static uint8_t idx;  // index for g_in_payload
  uint8_t in_byte;
  if (wifi_available()) {
    in_byte = wifi_get_byte();
    switch (g_in_status) {
      case IDLE:
        if (in_byte == CMD_START) {
          g_in_status = START;
        }
        break;
      case START:
        if (strchr(g_all_cmds, in_byte) != NULL) {  // check for valid command
          g_in_status = CMD;
          idx = 0;
          g_in_payload[idx] =
              in_byte;  // command byte is always the first char of payload
          idx++;
        } else {
          g_in_status = IDLE;  // reset
        }
        break;
      case CMD:
        if (in_byte == CMD_STOP) {  // stop byte found
          g_in_payload[idx] = '\0';
          command_dispatch(g_in_payload);   // process command now
          g_in_status = IDLE;               // reset
        } else if (in_byte == CMD_START) {  // found a re-start -> ignore
                                            // current incomplete cmd
          g_in_status = START;
        } else if (idx < CMD_PAYLOAD_SIZE - 1) {  // fill payload
          g_in_payload[idx] = in_byte;
          idx++;
        } else {
          g_in_status = IDLE;  // reset, buffer overflowed
        }
        break;
      default:
        g_in_status = IDLE;  // reset
    }
  }
}

// function called from command.c, passes outgoing data over wifi
void wifi_dispatch(const char *data) {
  wifi_send_byte(CMD_START);
  wifi_send(data);
  wifi_send_byte(CMD_STOP);
}
