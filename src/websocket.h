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
#ifndef WEBSOCKET_H_INCLUDED
#define WEBSOCKET_H_INCLUDED

// avr websocket server implementation
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

#include <stdint.h>
#include <string.h>

#include "base64_enc.h"
#include "sha1.h"
#include "usart_0.h"
#include "usart_1.h"

#include "command.h"

// proto's
void ws_init(void);
void ws_process(void);
void ws_dispatch(const char *data);

#endif
