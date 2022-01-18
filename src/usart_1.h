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
#ifndef USART_1_H_INCLUDED
#define USART_1_H_INCLUDED

// proto's
void uart_init_1(void);
void uart_put_1(uint8_t c);
void uart_put_str_1(const char *str);
void uart_put_str_P_1(const char *str);
void uart_put_int_1(const uint16_t dec);
uint8_t uart_get_1(void);
uint8_t uart_available_1(void);
void uart_flush_rx_1(void);
void uart_flush_tx_1(void);

#endif

