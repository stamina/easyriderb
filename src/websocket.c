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
#include "websocket.h"
// NOTE: limitations opposed to RFC: http://tools.ietf.org/html/rfc6455:
//
// - only uses text frames consisting of bytes (no UTF-8), no binary frames
// supported
// - only final frames allowed, no frame fragmentation support
// - maximum of 125 bytes as length of data (only using the first byte length
// indicator of the RFC spec)
// - no extensions support
// - no ping/pong frames support
// - handles only 1 client at a time (Wifly module supports only 1 active tcp/ip
// connection at a time)
//
/*0                   1                   2                   3*/
/*0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1*/
/*+-+-+-+-+-------+-+-------------+-------------------------------+*/
/*|F|R|R|R| opcode|M| Payload len |    Extended payload length    |*/
/*|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |*/
/*|N|V|V|V|       |S|             |   (if payload len==126/127)   |*/
/*| |1|2|3|       |K|             |                               |*/
/*+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +*/
/*|     Extended payload length continued, if payload len == 127  |*/
/*+ - - - - - - - - - - - - - - - +-------------------------------+*/
/*|                               |Masking-key, if MASK set to 1  |*/
/*+-------------------------------+-------------------------------+*/
/*| Masking-key (continued)       |          Payload Data         |*/
/*+-------------------------------- - - - - - - - - - - - - - - - +*/
/*:                     Payload Data continued ...                :*/
/*+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +*/
/*|                     Payload Data continued ...                |*/
/*+---------------------------------------------------------------+*/
#define CR_CHAR '\r'  // carriage return char
#define LF_CHAR '\n'  // line feed char

// handshake header check parts
#define HAS_UPGRADE_HDR 1
#define HAS_CONNECTION_HDR 2
#define HAS_VERSION_HDR 4
#define HAS_ORIGIN_HDR 8
#define HAS_HOST_HDR 16
#define HAS_KEY_HDR 32

static uint8_t g_valid_handshake;  // needs all header bits to be set for a
                                   // valid handshake is recognized
static char g_buffer[256];         // store incoming data from client
static char g_received_key[128];   // store incoming websocket key from client
static uint8_t g_current;          // pointer of current buffer position
static uint16_t g_errors;  // number of invalid bytes received, when expecting
                           // valid ws frames

typedef enum { CLOSE, CONNECTING, OPEN, CLOSING } t_ws_state;

typedef enum {
  WS_TEXT_FRAME = 0x01,  // utf-8 text frame
  WS_CLOSE_FRAME = 0x08  // close connection frame
} t_ws_opcode;

enum g_first_byte { WS_FINAL_TXT = 0x81, WS_FINAL_CLOSE = 0x88 };

typedef struct {
  uint8_t isFinal;
  uint8_t isMasked;
  t_ws_opcode opcode;
  uint8_t length;
  uint8_t mask[4];
  char data[126];
  uint8_t data_pointer;
} t_ws_frame;

static t_ws_state g_state = CLOSE;
static t_ws_frame g_in_frame;
static t_ws_frame g_out_frame;
static const t_ws_frame g_empty_frame;

static void ws_listen(void);
static uint8_t get_ws_handshake(void);
static uint8_t send_ws_handhake(void);
static uint8_t get_ws_frame(void);
static void send_ws_close_frame(void);
static void send_ws_frame(void);
void send_ws_text_frame(const char *data);

// usart function pointers
static void (*ws_send)(const char *str);
static void (*ws_send_byte)(uint8_t c);
static uint8_t (*ws_get_byte)(void);
static uint8_t (*ws_available)(void);
static void (*ws_flush)(void);

// websocket handshake header strings
static const char *g_host_field = "Host: ";
static const char *g_upgrade_field = "Upgrade: ";
static const char *g_connection_field = "Connection: ";
static const char *g_key_field = "Sec-WebSocket-Key: ";
static const char *g_origin_field = "Origin: ";
static const char *g_version_field = "Sec-WebSocket-Version: ";
static const char *g_versionnumber_field = "13";

static const char *g_return_status_field =
    "HTTP/1.1 101 Switching Protocols\r\n";
static const char *g_return_upgrade_field = "Upgrade: websocket\r\n";
static const char *g_return_connection_field = "Connection: Upgrade\r\n";
static const char *g_return_key_field = "Sec-WebSocket-Accept: ";
static const char *g_secret_key = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void ws_init(void) {
  ws_send = uart_put_str_0;
  ws_send_byte = uart_put_0;
  ws_get_byte = uart_get_0;
  ws_available = uart_available_0;
  ws_flush = uart_flush_rx_0;
}

// function called from command.c repetitively, handles all incoming websocket
// data
void ws_process(void) { ws_listen(); }

// function called from command.c, handles all outgoing websocket data
void ws_dispatch(const char *data) {
  if (g_state == OPEN) {
    send_ws_text_frame(data);
  }
}

static void ws_listen() {
  if (g_state == CLOSE) {
    if (get_ws_handshake()) {  // keep checking for valid handshake headers from
                               // client
                               /*#ifdef EASY_TRACE*/
      uart_put_str_1("g_state: CLOSE -> CONNECTING\n");
      /*#endif*/
      g_state = CONNECTING;
    }
  } else if (g_state == CONNECTING) {
    // valid handshake headers received, send handshake confirmation headers
    // back
    if (send_ws_handhake()) {
      // flush receive buffer (we're not interested in redundant extra HTTP
      // headers)
      // NOTE: there can still be some partial HTTP header bytes coming in, even
      // after the flush
      // get_ws_frame keep ignoring everything until the first real ws byte
      ws_flush();
      g_state = OPEN;  // HTTP is upgraded to ws at this point, time for
                       // websocket frame communication
                       /*#ifdef EASY_TRACE*/
      uart_put_str_1("g_state: CONNECTING -> OPEN\n");
      /*#endif*/
    } else {
      g_state = CLOSE;
      /*#ifdef EASY_TRACE*/
      uart_put_str_1("g_state: CONNECTING -> CLOSE\n");
      /*#endif*/
    }
  } else if (g_state == OPEN) {
    if (get_ws_frame()) {  // keep checking for valid frames from client
      // handle data as command
      command_dispatch(g_in_frame.data);
      /*#ifdef EASY_TRACE*/
      uart_put_str_1("command received: ");
      uart_put_str_1(g_in_frame.data);
      send_ws_text_frame(g_in_frame.data);
      /*#endif*/
      // reset to process next frame
      g_in_frame = g_empty_frame;
      /*#ifdef EASY_TRACE*/
      uart_put_str_1("g_state: OPEN\n");
      /*#endif*/
    }
  } else if (g_state == CLOSING) {
    // close request frame from client received or invalid frame data, send
    // close frame back
    send_ws_close_frame();
    ws_flush();
    g_state = CLOSE;
    /*#ifdef EASY_TRACE*/
    uart_put_str_1("g_state: CLOSING -> CLOSE\n");
    /*#endif*/
  }
}

// checks for a valid handshake
// Example header:
/* GET /chat HTTP/1.1*/
/* Host: server.example.com*/
/* Upgrade: websocket*/
/* Connection: Upgrade*/
/* Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==*/
/* Origin: http://example.com*/
/* Sec-WebSocket-Protocol: chat, superchat*/
/* Sec-WebSocket-Version: 13*/
static uint8_t get_ws_handshake() {
  if (ws_available()) {
    uint8_t cbyte;  // current byte
    cbyte = ws_get_byte();
    g_buffer[g_current++] = cbyte;
    // reached end of a possible header line or maximum of buffer
    if (cbyte == CR_CHAR || cbyte == LF_CHAR || g_current > 255) {
      g_buffer[g_current - 1] = '\0';  // terminate properly for strstr function
      g_current = 0;                   // reset for next http header line
      if (!(g_valid_handshake & HAS_HOST_HDR) &&
          strstr(g_buffer, g_host_field)) {
        g_valid_handshake |= HAS_HOST_HDR;
      } else if (!(g_valid_handshake & HAS_UPGRADE_HDR) &&
                 strstr(g_buffer, g_upgrade_field)) {
        g_valid_handshake |= HAS_UPGRADE_HDR;
      } else if (!(g_valid_handshake & HAS_CONNECTION_HDR) &&
                 strstr(g_buffer, g_connection_field)) {
        g_valid_handshake |= HAS_CONNECTION_HDR;
      } else if (!(g_valid_handshake & HAS_KEY_HDR) &&
                 strstr(g_buffer, g_key_field)) {
        g_valid_handshake |= HAS_KEY_HDR;
        // save incoming websocket key
        strtok(g_buffer, " ");
        strcpy(g_received_key, strtok(NULL, " "));
      } else if (!(g_valid_handshake & HAS_ORIGIN_HDR) &&
                 strstr(g_buffer, g_origin_field)) {
        g_valid_handshake |= HAS_ORIGIN_HDR;
      } else if (!(g_valid_handshake & HAS_VERSION_HDR) &&
                 strstr(g_buffer, g_version_field) &&
                 strstr(g_buffer, g_versionnumber_field)) {
        g_valid_handshake |= HAS_VERSION_HDR;
        ;
      }
    }
  }
  // found complete handshake
  if (g_valid_handshake ==
      (HAS_HOST_HDR | HAS_UPGRADE_HDR | HAS_CONNECTION_HDR | HAS_KEY_HDR |
       HAS_ORIGIN_HDR | HAS_VERSION_HDR)) {
    // reset first
    g_current = 0;
    g_valid_handshake = 0;
    return 1;
  }
  return 0;
}

// sends back the handshake confirmation headers
// Example header:
/* HTTP/1.1 101 Switching Protocols*/
/* Upgrade: websocket*/
/* Connection: Upgrade*/
/* Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=*/
static uint8_t send_ws_handhake() {
  // concatenate server key
  strcat(g_received_key, g_secret_key);
  // sha1 hash it
  char sha_hash[20];
  memset(sha_hash, 0, sizeof(sha_hash));
  sha1(sha_hash, g_received_key, strlen(g_received_key) * 8);
  // encode
  base64enc(g_received_key, sha_hash, 20);
  ws_send(g_return_status_field);
  ws_send(g_return_upgrade_field);
  ws_send(g_return_connection_field);
  ws_send(g_return_key_field);
  ws_send(g_received_key);
  ws_send("\r\n\r\n");
  return 1;
}

// prepares a close frame and delegates it to send_ws_frame
static void send_ws_close_frame() {
  g_out_frame.opcode = WS_CLOSE_FRAME;
  send_ws_frame();
}

// prepares a text frame and delegates it to send_ws_frame
void send_ws_text_frame(const char *data) {
  g_out_frame.opcode = WS_TEXT_FRAME;
  while (*data) {
    g_out_frame.data[g_out_frame.data_pointer++] = *data;
    data++;
  }
  g_out_frame.data[g_out_frame.data_pointer] = '\0';
  g_out_frame.length = g_out_frame.data_pointer;
  send_ws_frame();
}

// main websocket frame
static void send_ws_frame() {
  if ((g_out_frame.opcode == WS_TEXT_FRAME) && (g_out_frame.length <= 125) &&
      (g_state == OPEN)) {
    // send final text frame
    ws_send_byte(WS_FINAL_TXT);
    ws_send_byte(g_out_frame.length);
    for (int i = 0; i < g_out_frame.length; i++) {
      ws_send_byte(g_out_frame.data[i]);
    }
    g_out_frame = g_empty_frame;
    // send close frame
  } else if ((g_out_frame.opcode == WS_CLOSE_FRAME) &&
             ((g_state == OPEN) || (g_state == CLOSING))) {
    ws_send_byte(WS_FINAL_CLOSE);
    g_out_frame = g_empty_frame;
  }
}

// keep processing valid websocket frames from the client
static uint8_t get_ws_frame() {
  if (ws_available()) {
    uint8_t cbyte;  // current byte
    cbyte = ws_get_byte();
    // 1st byte check
    if (!g_in_frame
             .isFinal) {  // keep parsing bytes to reach a first valid ws byte
      if (cbyte == WS_FINAL_CLOSE) {
        // reset and close
        g_in_frame = g_empty_frame;
        g_state = CLOSING;
      } else if (cbyte == WS_FINAL_TXT) {
        g_in_frame.isFinal = 1;
        g_in_frame.opcode = WS_TEXT_FRAME;
      } else {  // still receiving invalid bytes, bailout if it's too much
        g_errors++;
        if (g_errors > 1024) {
          // reset and close
          g_errors = 0;
          g_in_frame = g_empty_frame;
          g_state = CLOSING;
        }
      }
      return 0;
      // 2nd byte checks
    } else if (!g_in_frame.isMasked) {
      // mask bit is mandatory and data <= 125 bytes
      g_in_frame.isMasked = cbyte & 0x80;
      g_in_frame.length = cbyte & 0x7F;
      if (!(g_in_frame.isMasked) || (g_in_frame.length > 125)) {
        // reset and close
        g_in_frame = g_empty_frame;
        g_state = CLOSING;
      } else {
        g_in_frame.isMasked = 1;
      }
      return 0;
      // 4 mask bytes needed, by reading each mask byte, increment isMasked
    } else if (g_in_frame.isMasked < 5) {
      g_in_frame.mask[g_in_frame.isMasked - 1] = cbyte;
      g_in_frame.isMasked++;
      return 0;
      // all mask bytes received, start gathering data bytes and unmask them
    } else if (g_in_frame.data_pointer < g_in_frame.length) {
      g_in_frame.data[g_in_frame.data_pointer] =
          cbyte ^ g_in_frame.mask[g_in_frame.data_pointer % 4];
      g_in_frame.data_pointer++;
      if (g_in_frame.data_pointer == g_in_frame.length) {
        // all data received, terminate string and start handling this command
        g_in_frame.data[g_in_frame.data_pointer] = '\0';
        return 1;
      }
      return 0;
    }
  }
  return 0;
}
