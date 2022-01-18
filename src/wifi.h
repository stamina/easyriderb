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
#ifndef WIFI_H_INCLUDED
#define WIFI_H_INCLUDED

#include <stdint.h>
#include <string.h>
#include "usart_0.h"
#include "usart_1.h"

#include "command.h"

// proto's
void wifi_init(void);
void wifi_process(void);
void wifi_dispatch(const char *data);

#endif
