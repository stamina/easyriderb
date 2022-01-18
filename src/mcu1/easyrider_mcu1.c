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
#include "easyrider_mcu1.h"

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../command.h"
#include "sound.h"

// callbacks struct array: main handler functions and pre-guard functions for
// the queued events
tCallBack g_callbacks[] = {
    {EV_READ_BATTERY, check_battery_read, process_battery},
    {EV_READ_TEMPERATURE, check_temperature_read, process_temperature},
    {EV_READ_CURRENT, check_board_current, process_board_current},
    {EV_READ_ACCEL, check_board_accel, process_board_accel},
    {EV_G1_OFF, check_gear1_off, process_gear1_off},
    {EV_G2_OFF, check_gear2_off, process_gear2_off},
    {EV_G3_OFF, check_gear3_off, process_gear3_off},
    {EV_G4_OFF, check_gear4_off, process_gear4_off},
    {EV_G1_ON, check_gear1_on, process_gear1_on},
    {EV_G2_ON, check_gear2_on, process_gear2_on},
    {EV_G3_ON, check_gear3_on, process_gear3_on},
    {EV_G4_ON, check_gear4_on, process_gear4_on}};

// Gets called from the main loop, generates events and puts them in the event
// queue. The complete queue is handled (emptied) in each main loop iteration in
// a FIFO way, so better put the higher priority events at the top
static void dispatch_events() {
  check_battery();
  check_current();
  check_temperature();
  check_accel();
  check_gear1();
  check_gear2();
  check_gear3();
  check_gear4();
  check_neutral();
  check_sound();
}

// default function for checking sense pins and dispatching events
void check_default(uint8_t event_on, uint8_t event_off, uint8_t pin,
                   volatile uint8_t *debounce_timer_flag,
                   volatile uint8_t *debounce_state) {
  // keep checking for 6 positives in a short timespan, so we're certain a
  // button is pressed/released and it's not noise
  if (*debounce_timer_flag) {  // timer-based flag is set (again)
    *debounce_timer_flag = 0;  // reset to wait for next timer event
    if (pin) {
      *debounce_state =
          (*debounce_state << 1);  // add 0 as a positive "off" press
      if ((*debounce_state & 0b00111111) == 0) {
        set_event(event_off);
      }
    } else {
      *debounce_state =
          (*debounce_state << 1) | 1;  // add 1 as a positive "on" press
      if ((*debounce_state & 0b00111111) == 0b00111111) {
        set_event(event_on);
      }
    }
  }
}

void check_gear1() {
  check_default(EV_G1_ON, EV_G1_OFF, STATUS_C90_SENSE_G1, &FLAG_DEBOUNCE_G1,
                &g_gear1_debounce);
}

void check_gear2() {
  check_default(EV_G2_ON, EV_G2_OFF, STATUS_C90_SENSE_G2, &FLAG_DEBOUNCE_G2,
                &g_gear2_debounce);
}

void check_gear3() {
  check_default(EV_G3_ON, EV_G3_OFF, STATUS_C90_SENSE_G3, &FLAG_DEBOUNCE_G3,
                &g_gear3_debounce);
}

void check_gear4() {
  check_default(EV_G4_ON, EV_G4_OFF, STATUS_C90_SENSE_G4, &FLAG_DEBOUNCE_G4,
                &g_gear4_debounce);
}

void check_neutral() {
  if (g_neutral_counter >= 100) {  // every 0.5s (5ms*100) check
    g_neutral_counter = 0;
    if (g_gear) {
      set_gear(0);  // only set neutral when not in neutral already
    }
  }
}

void check_battery() { set_event(EV_READ_BATTERY); }

void check_current() { set_event(EV_READ_CURRENT); }

void check_temperature() { set_event(EV_READ_TEMPERATURE); }

void check_accel() { set_event(EV_READ_ACCEL); }

uint8_t check_battery_read() { return 1; }

uint8_t check_board_current() { return 1; }

uint8_t check_temperature_read() { return 1; }

uint8_t check_board_accel() { return 1; }

uint8_t check_gear1_on() { return (g_current_gear != 1); }

uint8_t check_gear2_on() { return (g_current_gear != 2); }

uint8_t check_gear3_on() { return (g_current_gear != 3); }

uint8_t check_gear4_on() { return (g_current_gear != 4); }

uint8_t check_gear1_off() { return (g_current_gear == 1); }

uint8_t check_gear2_off() { return (g_current_gear == 2); }

uint8_t check_gear3_off() { return (g_current_gear == 3); }

uint8_t check_gear4_off() { return (g_current_gear == 4); }

// get the next event from the event queue (circular buffer)
uint8_t get_event() {
  uint8_t ev;
  if (g_buffer_tail != g_buffer_head) {  // not empty
    ev = g_event_buffer[g_buffer_tail];
    g_buffer_tail++;
    if (g_buffer_tail >= C90_EVENT_BUFFER_SIZE) {
      g_buffer_tail = 0;
    }
    return ev;
  } else {
    return EV_VOID;  // no events, return a void event
  }
}

// put an event in the event queue (circular buffer)
void set_event(uint8_t ev) {
  g_event_buffer[g_buffer_head] = ev;  // insert event at head position
  g_buffer_head++;                     // advance head
  if (g_buffer_head >= C90_EVENT_BUFFER_SIZE) {
    g_buffer_head = 0;  // cycle back to start
  }
  if (g_buffer_head == g_buffer_tail) {
    g_buffer_tail++;  // also move tail, basically destroying the oldest event
                      // to make space
  }
  if (g_buffer_tail >= C90_EVENT_BUFFER_SIZE) {
    g_buffer_tail = 0;  // cycle back to start
  }
}

// sets the gear
static void set_gear(uint8_t gear) { g_gear = gear; }

// calculates the battery's voltage
// Vref = 5v, if Vref isnt exactly 5.00v, but a bit off, tweak
// C90_OFFSET_ADC_READING for 12v battery readout: my voltage divider
// ratio: 2.2K - 1K Vmeasure:  0.3125 * Vbat 10bit ADCvalue (0-1023):
// Vmeasure/(5/1024) Vbat: (ADCvalue*(5/1024))/(0.3125)
void process_battery() {
  if (FLAG_READ_BATTERY) {
    g_voltage = 49 * g_adc_voltage[3] / 3.125;
    FLAG_READ_BATTERY = 0;
  }
}

// calculates the current consumption of the board in mA, according the sensor's
// voltage output sampled by the ADC default output of the sensor at 0mA
// is 2.5V, 512 ADC, since it can measure AC current and it's hooked up in
// reverse, this value drops when the current increases (using the 0-512 10bit
// part instead of 512-1023) 10bit ADCvalue (0-1023): (5v/1024) -> 0.00488 volt
// per bit resolution sensor: 185mV/A -> 0.0185 volt == 100mA -> 0.00488/0.0185
// == 26mA per bit -> due to low res/noise set to a sensible 25mA/bit
void process_board_current() {
  if (FLAG_READ_CURRENT) {
    g_current = abs(512 - g_adc_voltage[4]) * 25;
    FLAG_READ_CURRENT = 0;
  }
}

// calculates the board's ambient temperature using an LM35 sensor which gives
// 10mV/Celsius since 1 bit is roughly 5mV, max resolution is 0.5 degrees Vref =
// 5v, if Vref isnt exactly 5.00v, but a bit off, tweak C90_OFFSET_ADC_READING
// 10bit ADCvalue (0-1023): Vmeasure/(5/1024) -> use 49*voltage == temperature
// (divide by 100) on receiver side
void process_temperature() {
  if (FLAG_READ_TEMPERATURE) {
    g_temperature = (49 * g_adc_voltage[5]);
    FLAG_READ_TEMPERATURE = 0;
  }
}

// reads the accelerometer's x-/y-/z-axis
// 1.5g setting:  Sensitivity: 800 mV/g  -1g = 850mV  0g = 1650mV 1g = 2450mV
// Vref = 5v, if Vref isnt exactly 5.00v, but a bit off, tweak
// C90_OFFSET_ADC_READING 10bit ADCvalue (0-1023): Vmeasure/(5/1024)
void process_board_accel() {
  if (FLAG_READ_ACCEL) {
    g_accelx = g_adc_voltage[0];
    g_accely = g_adc_voltage[1];
    g_accelz = g_adc_voltage[2];
    FLAG_READ_ACCEL = 0;
  }
}

void process_gear1_on() {
  set_gear(1);
  g_current_gear = 1;
}

void process_gear1_off() {
  g_current_gear = 0;  // assume a neutral until next gear_on changes it again
}

void process_gear2_on() {
  set_gear(2);
  g_current_gear = 2;
}

void process_gear2_off() {
  g_current_gear = 0;  // assume a neutral until next gear_on changes it again
}

void process_gear3_on() {
  set_gear(3);
  g_current_gear = 3;
}

void process_gear3_off() {
  g_current_gear = 0;  // assume a neutral until next gear_on changes it again
}

void process_gear4_on() {
  set_gear(4);
  g_current_gear = 4;
}

void process_gear4_off() {
  g_current_gear = 0;  // assume a neutral until next gear_on changes it again
}

void init_ports() {
  // gear sense pins DDR/PORT: as input(0)/high(1)
  DDR_C90_SENSE_G1 &= ~(1 << PIN_C90_SENSE_G1);
  PORT_C90_SENSE_G1 |= (1 << PIN_C90_SENSE_G1);
  DDR_C90_SENSE_G2 &= ~(1 << PIN_C90_SENSE_G2);
  PORT_C90_SENSE_G2 |= (1 << PIN_C90_SENSE_G2);
  DDR_C90_SENSE_G3 &= ~(1 << PIN_C90_SENSE_G3);
  PORT_C90_SENSE_G3 |= (1 << PIN_C90_SENSE_G3);
  DDR_C90_SENSE_G4 &= ~(1 << PIN_C90_SENSE_G4);
  PORT_C90_SENSE_G4 |= (1 << PIN_C90_SENSE_G4);
  // RPM T0 counter pin as as input(0)/high(1)
  DDR_C90_RPM &= ~(1 << PIN_C90_RPM);
  PORT_C90_RPM |= (1 << PIN_C90_RPM);
  // heartbeat led on pcb
  DDR_C90_HEARTBEAT_LED |= (1 << PIN_C90_HEARTBEAT_LED);
  PORT_C90_HEARTBEAT_LED |= (1 << PIN_C90_HEARTBEAT_LED);
  // buzzer
  DDR_C90_BUZZER |= (1 << PIN_C90_BUZZER);    // output
  PORT_C90_BUZZER &= ~(1 << PIN_C90_BUZZER);  // low
}

// setup the ADC in Free Running Mode (3 LSB in ADCSRB: 0 by default), so we can
// advance the read pin ourselves
void enable_adc() {
  // analog input channel selections,  clear the bottom 3 bits before setting
  // the new pin for readout the 3 LSB select the currently active  ADC0-7
  ADMUX = (ADMUX & 0xF8) | g_adc_read_pin;
  ADMUX |= (1 << REFS0);  // voltage reference = AVCC
  DIDR0 = 0x3F;           // 0b00111111, disable digital inputs for ADC0-ADC5
  ADCSRA |= (1 << ADEN);  // enable ADC
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // prescaler = 128
  ADCSRA |= (1 << ADIE);                                 // enable interrupt
  ADCSRA |= (1 << ADSC);                                 // start a conversion
}

ISR(ADC_vect) {
  // first read ADCL, then ADCH to keep it an atomic operation
  g_adc_voltage[g_adc_read_pin] = ADCL;
  g_adc_voltage[g_adc_read_pin] =
      (g_adc_voltage[g_adc_read_pin] | (ADCH << 8));  // ADCH has the last 2
                                                      // bits as LSB, which
                                                      // become MSB in uint16_t
  g_adc_voltage[g_adc_read_pin] += C90_OFFSET_ADC_READING;
  g_adc_read_pin++;  // increment pin to read a new voltage on
  if (g_adc_read_pin > 5) {
    g_adc_read_pin = 0;
  }
  ADMUX = (ADMUX & 0xF8) | g_adc_read_pin;
  ADCSRA |= (1 << ADSC);  // start another conversion again
}

// RPM timer
void start_rpm_timer() {
  // Configure timer 0 (8-bit) for CTC mode (Clear on Timer Compare)
  // set timer as CTC Mode
  TCCR0A |= (1 << WGM01);
  TCCR0A |= ~(1 << WGM00);
  TCCR0A &= ~((1 << COM0A1) |
              (1 << COM0A0));  // DON'T use PB3's OC0A functionality, else it
                               // will interfere with the buzzer on that pin
  TCCR0B |= ~(1 << WGM02);
  // no prescale, but configure as external clock source on T0 pin. Clock on
  // falling edge.
  TCCR0B |= ((1 << CS01) | (1 << CS02));
  TCCR0B &= ~(1 << CS00);
  TIMSK0 |= (1 << OCIE0A);  // Enable Compare A interrupt
  OCR0A = 1;  // Set CTC compare A value to 1, to calculate the time between
              // each pulse
}

void start_buzzer_timer() {
  // Configure timer 1 (16-bit) for CTC mode (Clear on Timer Compare)
  TCCR1A &= ~((1 << WGM11) | (1 << WGM10));  // CTC Mode
  TCCR1B |= (1 << WGM12);                    // CTC Mode
  TCCR1B |= ((1 << CS11) | (1 << CS10));     // prescale 64
  TCCR1B &= ~(1 << CS12);                    // prescale 64
  // Set CTC compare value, approx. 4600hz: with a square wave that is a 2300hz
  // tone This is the default value, when playing sounds, this value is
  // constantly updated with the tone's frequency
  OCR1A = 68;
}

// sense timer for debouncing the gear senses
void start_sense_timer() {
  // Configure timer 2 (8-bit) for CTC mode (Clear on Timer Compare)
  // set timer as CTC Mode
  TCCR2A |= (1 << WGM21);
  TCCR2A |= ~(1 << WGM20);
  TCCR2B |= ~(1 << WGM22);
  // prescale 1024
  TCCR2B |= ((1 << CS20) | (1 << CS21) | (1 << CS22));
  TIMSK2 |= (1 << OCIE2A);  // Enable Compare A interrupt
  OCR2A = 97;  // Set CTC compare A value, approx. 5 ms -> 200 times/sec
}

// timer for the period calculation of the RPM
void start_10us_timer() {
  // Configure timer 3 (16-bit) for CTC mode (Clear on Timer Compare)
  TCCR3A &= ~((1 << WGM31) | (1 << WGM30));  // CTC Mode
  TCCR3B |= (1 << WGM32);                    // CTC Mode
  // prescale with 8 to get 2.5million hz with a 20mhz crystal
  TCCR3B &= ~((1 << CS30) | (1 << CS32));
  TCCR3B |= (1 << CS31);
  TIMSK3 |= (1 << OCIE3A);  // Enable Compare A interrupt
  OCR3A = 24;               // Set CTC compare value, 100.000 times/sec
}

// interrupt handler for 8bit timer0 that triggers every RPM pulse
// resolution of RPM is 120RPM per bit steps
ISR(TIMER0_COMPA_vect) {
  // calculate RPM by checking how many 10 microsecond ticks have passed since
  // last pulse, the multiply by 2 is needed since we only detect the positive
  // pulse of the rectified signal
  g_rpm = (100000UL / g_10us_ticks * 60) * 2;
  // reset ticks
  g_10us_ticks = 0;
  // reset to full to notify that we still have rpms
  g_rpm_reset = 40;
}

// timer interrupt for the speaker
ISR(TIMER1_COMPA_vect) {
  // toggle buzzer pin to create a 50% duty cycle
  PORT_C90_BUZZER ^= (1 << PIN_C90_BUZZER);
}

// timer interrupt for the sense, each 5ms
ISR(TIMER2_COMPA_vect) {
  // reset rpm to 0 when engine stalls
  if (g_rpm_reset > 0) {
    g_rpm_reset--;
  } else if (g_rpm) {
    g_rpm = 0;
  }
  // neutral check after g_current_gear == 0 for # counts
  if (!g_current_gear) {
    g_neutral_counter++;
  } else {
    g_neutral_counter = 0;
  }
#ifdef EASY_TRACE123
  g_uart_display_counter++;
#endif
  FLAG_READ_BATTERY = 1;
  FLAG_READ_CURRENT = 1;
  FLAG_READ_TEMPERATURE = 1;
  FLAG_READ_ACCEL = 1;
  FLAG_DEBOUNCE_G1 = 1;
  FLAG_DEBOUNCE_G2 = 1;
  FLAG_DEBOUNCE_G3 = 1;
  FLAG_DEBOUNCE_G4 = 1;
  if (g_music_duration) {
    g_music_duration--;
  }
}

// timer interrupt with 10 microsecond intervals to calculate RPM
ISR(TIMER3_COMPA_vect) { g_10us_ticks++; }

void check_sound() {
  if (FLAG_MUSIC) {
    if (!g_music_duration) {
      if (pgm_read_word(g_current_music)) {  // get note data until MUSIC_END
        if (g_current_music ==
            g_selected_music) {  // still at start: first byte is the tempo
          g_music_tempo = pgm_read_word(g_current_music);
          g_current_music++;
        }
        g_music_duration =
            calc_note_duration(pgm_read_word(g_current_music), g_music_tempo);
        g_current_music++;
        if (pgm_read_word(g_current_music) ==
            MUSIC_P) {               // pause check (silence for a certain time)
          TIMSK1 &= ~(1 << OCIE1A);  // disable interrupt
          PORT_C90_BUZZER &= ~(1 << PIN_C90_BUZZER);
        } else {
          TIMSK1 |= (1 << OCIE1A);  // enable interrupt
        }
        OCR1A = pgm_read_word(g_current_music);
        TCNT1 = 0;
        g_current_music++;
      } else {  // the end of song
        FLAG_MUSIC = 0;
        TIMSK1 &= ~(1 << OCIE1A);  // disable interrupt
        PORT_C90_BUZZER &= ~(1 << PIN_C90_BUZZER);
      }
    }
  }
}

void set_sound(uint8_t status) {
  uint8_t song_idx;
  // initially mute current sound
  TIMSK1 &= ~(1 << OCIE1A);  // disable interrupt
  PORT_C90_BUZZER &= ~(1 << PIN_C90_BUZZER);
  FLAG_MUSIC = 0;
  switch (status) {
    case 255:  // random song
      srand(g_adc_voltage[0] + g_adc_voltage[1] + g_adc_voltage[2] +
            g_adc_voltage[3]);               // random seed
      song_idx = 1 + (uint8_t)(rand() % 5);  // add 0-4 idx
      g_music_duration = 0;
      g_current_music = (uint16_t *)pgm_read_word(&g_music[song_idx]);
      g_selected_music = (uint16_t *)pgm_read_word(&g_music[song_idx]);
      FLAG_MUSIC = 1;
      break;
    case 254:  // no sound, do nothing
      break;
    case 253:  // beep only
      OCR1A = 68;
      TCNT1 = 0;
      TIMSK1 |= (1 << OCIE1A);  // enable interrupt
      break;
    default:  // specific idx
      g_music_duration = 0;
      g_current_music = (uint16_t *)pgm_read_word(&g_music[status]);
      g_selected_music = (uint16_t *)pgm_read_word(&g_music[status]);
      FLAG_MUSIC = 1;
  }
}

void initialize() {
  MCUCR = 0x80;  // disable JTAG at runtime (2 calls in a row needed)
  MCUCR = 0x80;
  wdt_enable(WDTO_2S);                // enable 2 sec watchdog
  sei();                              // enable global interrupts
  g_buffer_head = g_buffer_tail = 0;  // init event buffer
  enable_adc();
  init_ports();
  start_rpm_timer();
  start_buzzer_timer();
  start_sense_timer();
  start_10us_timer();
  command_init();
}

int main(void) {
  initialize();
  while (1) {
    dispatch_events();
    while ((g_event = get_event()) !=
           EV_VOID) {  // handle the complete event queue in one go
      for (uint8_t i = 0; i < TR_CNT; i++) {
        if (g_event == g_callbacks[i].event) {  // check if event matches
          if (g_callbacks[i].check_func()) {    // guard condition to check if
                                              // current state is accepting this
                                              // event
            g_callbacks[i].process_func();  // call event handler
            break;
          }
        }
      }
    }
    //  WiFi serial wifly passthrough of UART1 cable -> UART0 Wifi (mcu1)
    //  or
    //  GPS passthrough of UART1 cable -> UART0 GPS (mcu2)
    /*if (uart_available_1()) {*/
    /*uart_put_0(uart_get_1());*/
    /*}*/
    /*if (uart_available_0()) {*/
    /*uart_put_1(uart_get_0());*/
    /*}*/
    // process all commands (SPI, UART and WIFI)
    command_process();
#ifdef EASY_TRACE123
    if (g_uart_display_counter >= 20) {
      g_uart_display_counter = 0;
      uart_put_str_1("accelx: ");
      uart_put_int_1(g_accelx);
      uart_put_str_1("\r\n");
      uart_put_str_1("accely: ");
      uart_put_int_1(g_accely);
      uart_put_str_1("\r\n");
      uart_put_str_1("accelz: ");
      uart_put_int_1(g_accelz);
      uart_put_str_1("\r\n");
      uart_put_str_1("voltage: ");
      uart_put_int_1(g_voltage);
      uart_put_str_1("\r\n");
      uart_put_str_1("current: ");
      uart_put_int_1(g_current);
      uart_put_str_1("\r\n");
      uart_put_str_1("temperature: ");
      uart_put_int_1(g_temperature);
      uart_put_str_1("\r\n");
      uart_put_str_1("rpm: ");
      uart_put_int_1(g_rpm);
      uart_put_str_1("\r\n");
      uart_put_str_1("gear: ");
      uart_put_int_1(g_gear);
      uart_put_str_1("\r\n");
    }
#endif
    if (!g_mcu_reset) {
      wdt_reset();  // reset the watchdog, i.e. don't reset the mcu
    }
  }
  return 0;  // keep C-compiler happy, although will never reach this point
}
