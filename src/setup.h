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
 */

//  WiFi serial wifly passthrough of UART1 cable -> UART0 Wifi (mcu1)
//  or
//  GPS passthrough of UART1 cable -> UART0 GPS (mcu2)
if (uart_available_1()) {
  uart_put_0(uart_get_1());
}
if (uart_available_0()) {
  uart_put_1(uart_get_0());
}

#ifdef EASY_SETUP
  // RTC
  ds1307_start_osc();
  g_datetime.weekday = 4;
  g_datetime.day = 1;
  g_datetime.month = 10;
  g_datetime.year = 15;
  g_datetime.hours = 22;
  g_datetime.minutes = 27;
  g_datetime.seconds = 0;
  ds1307_set_date(&g_datetime); // set inital date/time
  _delay_ms(100);
  // GPS
  // 57600 baud
  uart_put_0(160);
  uart_put_0(161);
  uart_put_0(0x00);
  uart_put_0(0x04);
  uart_put_0(0x05);
  uart_put_0(0x00);
  uart_put_0(0x04);
  uart_put_0(0x01);
  uart_put_0(0x00);
  uart_put_0(13);
  uart_put_0(10);
  _delay_ms(100);
  // Binary only output
  uart_put_0(160);
  uart_put_0(161);
  uart_put_0(0x00);
  uart_put_0(0x03);
  uart_put_0(0x09);
  uart_put_0(0x02);
  uart_put_0(0x01);
  uart_put_0(10);
  uart_put_0(13);
  uart_put_0(10);
  _delay_ms(100);
  // 10 hz update rate
  uart_put_0(160);
  uart_put_0(161);      
  uart_put_0(0x00);
  uart_put_0(0x03);
  uart_put_0(14);
  uart_put_0(10);
  uart_put_0(0x01);
  uart_put_0(0x05);
  uart_put_0(13);
  uart_put_0(10);
  _delay_ms(100);
#endif
