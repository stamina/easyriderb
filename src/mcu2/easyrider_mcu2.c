/*
 *
 *  Copyright (C) Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
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
#include <util/delay.h>

#include "easyrider_mcu2.h"

// state transition struct array: main handler functions and pre-guard functions
// for the queued events
tTransition g_trans[] = {
    {EV_BRAKE_ON, check_brake_on, process_brake_on},
    {EV_BRAKE_OFF, check_brake_off, process_brake_off},
    {EV_CLAXON_ON, check_claxon_on, process_claxon_on},
    {EV_CLAXON_OFF, check_claxon_off, process_claxon_off},
    {EV_RI_ON, check_ri_on, process_ri_on},
    {EV_RI_OFF, check_ri_off, process_ri_off},
    {EV_LI_ON, check_li_on, process_li_on},
    {EV_LI_OFF, check_li_off, process_li_off},
    {EV_WARNING_ON, check_warning_on, process_warning_on},
    {EV_WARNING_OFF, check_warning_off, process_warning_off},
    {EV_IGN_ON, check_ign_on, process_ign_on},
    {EV_IGN_OFF, check_ign_off, process_ign_off},
    {EV_PILOT_ON, check_pilot_on, process_pilot_on},
    {EV_PILOT_OFF, check_pilot_off, process_pilot_off},
    {EV_LIGHT_ON, check_light_on, process_light_on},
    {EV_LIGHT_OFF, check_light_off, process_light_off},
    {EV_NEUTRAL_ON, check_neutral_on, process_neutral_on},
    {EV_NEUTRAL_OFF, check_neutral_off, process_neutral_off},
    {EV_ALARM_ON, check_alarm_on, process_alarm_on},
    {EV_ALARM_OFF, check_alarm_off, process_alarm_off}};

static void set_state(uint16_t st) {
  g_state = st;
  command_trigger_state(CMD_IF_IC);
#ifdef EASY_TRACE
  command_trigger_state(CMD_IF_DEBUG);
#endif
}

static uint16_t get_substate(uint16_t st) { return (g_state & st); }

static void set_substate(uint16_t st) {
  g_state |= st;
  command_trigger_state(CMD_IF_IC);
#ifdef EASY_TRACE
  command_trigger_state(CMD_IF_DEBUG);
#endif
}

static void remove_substate(uint16_t st) {
  g_state &= ~st;
  command_trigger_state(CMD_IF_IC);
#ifdef EASY_TRACE
  command_trigger_state(CMD_IF_DEBUG);
#endif
}

// Gets called from the main loop, reads all senses and puts them in the event
// queue. The complete queue is handled (emptied) in each main loop iteration in
// a FIFO way, so better put the higher priority events at the top, i.e. claxon
// sound is more important then indicator light.
static void dispatch_events() {
  check_brake();   // high prio, critical
  check_claxon();  // high prio, critical
  check_light();
  check_ri();
  check_li();
  check_warning();
  check_pilot();
  check_alarm();
  check_alarm_settle();
  check_alarm_trigger();
  check_neutral();
  check_rtc();
  check_stats();
  check_gps();
  check_ign();
}

uint8_t check_brake_on() {
  return ((g_state & ~(ST_BRAKE | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_brake_off() { return (g_state & ST_BRAKE); }

uint8_t check_claxon_on() {
  return ((g_state & ~(ST_CLAXON | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_claxon_off() { return (g_state & ST_CLAXON); }

uint8_t check_pilot_on() {
  return ((g_state & ~(ST_PILOT | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_pilot_off() { return (g_state & ST_PILOT); }

uint8_t check_light_on() {
  return ((g_state & ~(ST_LIGHT | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_light_off() { return (g_state & ST_LIGHT); }

uint8_t check_neutral_on() {
  return ((g_state & ~(ST_NEUTRAL | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_neutral_off() { return (g_state & ST_NEUTRAL); }

uint8_t check_ign_on() {
  return ((g_state & (ST_SLEEP | ST_ALARM)) == g_state);
}

uint8_t check_ign_off() {
  return ((g_state & ~(ST_SLEEP | ST_ALARM)) == g_state);
}

uint8_t check_ri_on() {
  return ((g_state & ~(ST_WARNING | ST_LI | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_ri_off() { return (g_state & ST_RI); }

uint8_t check_li_on() {
  if (SPCR & (1 << SPE))
    return 0;  // extra check to ignore SI_LI when SPI transfer is active, since
               // this SS pin is an output(1) during SPI hardware mode
  return ((g_state & ~(ST_WARNING | ST_RI | ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_li_off() {
  if (SPCR & (1 << SPE))
    return 0;  // extra check to ignore SI_LI when SPI transfer is active, since
               // this SS pin is an output(1) during SPI hardware mode
  return (g_state & ST_LI);
}

// warning blinks override RI/LI
uint8_t check_warning_on() {
  return ((g_state & ~(ST_ALARM | ST_SLEEP)) == g_state);
}

uint8_t check_warning_off() { return (g_state & ST_WARNING); }

uint8_t check_alarm_on() {
  return ((g_state & ST_ACTIVE) && ((g_state & ~ST_ALARM_SET) == g_state));
}

uint8_t check_alarm_off() {
  return ((g_state & ST_ALARM_SET) &&
          ((g_state & ~(ST_ALARM | ST_SLEEP)) == g_state));
}

// default function for checking sense pins and dispatching events
void check_default(uint16_t sense_flag, uint8_t event_on, uint8_t event_off,
                   uint8_t pin, volatile uint8_t *debounce_timer_flag,
                   volatile uint8_t *debounce_state) {
  // physical senses
  if ((g_settings.p_senses_active & sense_flag) == sense_flag) {
    // keep checking for 6 positives in a short timespan, so we're certain a
    // button is pressed/released and it's not noise timespan is 6*5ms, so the
    // delay is 30ms
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
  // dynamic senses
  if ((g_settings.d_senses_active & sense_flag) == sense_flag) {
    if ((g_d_senses_status & sense_flag) == sense_flag) {
      set_event(event_on);
    } else {
      set_event(event_off);
    }
  }
}

void check_brake() {
  check_default(FLAG_SENSE_BRAKE, EV_BRAKE_ON, EV_BRAKE_OFF,
                STATUS_C90_SENSE_BRAKE, &FLAG_DEBOUNCE_BRAKE,
                &g_brake_debounce);
}

void check_claxon() {
  check_default(FLAG_SENSE_CLAXON, EV_CLAXON_ON, EV_CLAXON_OFF,
                STATUS_C90_SENSE_CLAXON, &FLAG_DEBOUNCE_CLAXON,
                &g_claxon_debounce);
}

void check_pilot() {
  check_default(FLAG_SENSE_PILOT, EV_PILOT_ON, EV_PILOT_OFF,
                STATUS_C90_SENSE_PILOT, &FLAG_DEBOUNCE_PILOT,
                &g_pilot_debounce);
}

void check_light() {
  check_default(FLAG_SENSE_LIGHT, EV_LIGHT_ON, EV_LIGHT_OFF,
                STATUS_C90_SENSE_LIGHT, &FLAG_DEBOUNCE_LIGHT,
                &g_light_debounce);
}

void check_ign() {
  check_default(FLAG_SENSE_IGN, EV_IGN_ON, EV_IGN_OFF, STATUS_C90_SENSE_IGN,
                &FLAG_DEBOUNCE_IGN, &g_ign_debounce);
}

void check_ri() {
  check_default(FLAG_SENSE_LIGHT_RI, EV_RI_ON, EV_RI_OFF,
                STATUS_C90_SENSE_LIGHT_RI, &FLAG_DEBOUNCE_RI, &g_ri_debounce);
}

void check_li() {
  check_default(FLAG_SENSE_LIGHT_LI, EV_LI_ON, EV_LI_OFF,
                STATUS_C90_SENSE_LIGHT_LI, &FLAG_DEBOUNCE_LI, &g_li_debounce);
}

void check_warning() {
  check_default(FLAG_SENSE_WARNING, EV_WARNING_ON, EV_WARNING_OFF,
                STATUS_C90_SENSE_WARNING, &FLAG_DEBOUNCE_WARNING,
                &g_warning_debounce);
}

void check_alarm() {
  check_default(FLAG_SENSE_ALARM, EV_ALARM_ON, EV_ALARM_OFF,
                STATUS_C90_SENSE_ALARM, &FLAG_DEBOUNCE_ALARM,
                &g_alarm_debounce);
}

/*void check_alarm_settle() {*/
/*[>if (g_state == ST_ALARM_SETTLE) {<]*/
/*[>if (!FLAG_ALARM_SETTLE) { // first time in alarm settle mode<]*/
/*[>uint8_t song_idx = g_settings.alarm_sound;<]*/
/*[>// check for random startup song<]*/
/*[>if (song_idx == 0) {<]*/
/*[>srand(g_adc_voltage[0] + g_adc_voltage[1] + g_adc_voltage[2] +
 * g_adc_voltage[3]); // kinda random seed<]*/
/*[>song_idx = 1 + (uint8_t)(rand() / (RAND_MAX / 6));<]*/
/*[>}<]*/
/*[>FLAG_ALARM_SETTLE = 1;<]*/
/*[>// activate indicators<]*/

// alle relays functie hier gebruiken

/*[>PORT_C90_LIGHT_INDICATOR_COCKPIT |= (1 <<
 * PIN_C90_LIGHT_INDICATOR_COCKPIT);<]*/
/*[>PORT_C90_LIGHT_RI_F |= (1 << PIN_C90_LIGHT_RI_F);<]*/
/*[>PORT_C90_LIGHT_RI_B |= (1 << PIN_C90_LIGHT_RI_B);<]*/
/*[>PORT_C90_LIGHT_LI_F |= (1 << PIN_C90_LIGHT_LI_F);<]*/
/*[>PORT_C90_LIGHT_LI_B |= (1 << PIN_C90_LIGHT_LI_B);<]*/
/*[>g_music_duration = 0;<]*/
/*[>g_current_music = (uint16_t*)pgm_read_word(&g_music[song_idx-1]);<]*/
/*[>g_selected_music = (uint16_t*)pgm_read_word(&g_music[song_idx-1]);<]*/
/*[>FLAG_MUSIC = 1;<]*/
/*[>g_current_alarm_settle_time = g_settings.alarm_settle_time;<]*/
/*[>g_alarm_snapshot[0] = 0;<]*/
/*[>g_alarm_snapshot[1] = 0;<]*/
/*[>g_alarm_snapshot[2] = 0;<]*/
/*[>}<]*/
/*[>// save the AVERAGE accelerometer values as the alarm trigger reference
 * during the settlement<]*/
/*[>if (g_alarm_snapshot[0]) {<]*/
/*[>g_alarm_snapshot[0] = ((g_alarm_snapshot[0] + g_adc_voltage[1])/2);<]*/
/*[>} else {<]*/
/*[>g_alarm_snapshot[0] = g_adc_voltage[1];<]*/
/*[>}<]*/
/*[>if (g_alarm_snapshot[1]) {<]*/
/*[>g_alarm_snapshot[1] = ((g_alarm_snapshot[1] + g_adc_voltage[2])/2);<]*/
/*[>} else {<]*/
/*[>g_alarm_snapshot[1] = g_adc_voltage[2];<]*/
/*[>}<]*/
/*[>if (g_alarm_snapshot[2]) {<]*/
/*[>g_alarm_snapshot[2] = ((g_alarm_snapshot[2] + g_adc_voltage[3])/2);<]*/
/*[>} else {<]*/
/*[>g_alarm_snapshot[2] = g_adc_voltage[3];<]*/
/*[>}<]*/
/*[>// alarm settle time is over: only start alarm with valid accelerometer
 * voltages<]*/
/*[>if ((!g_current_alarm_settle_time) && ((g_alarm_snapshot[0] >
 * g_settings.alarm_thres_min) && (g_alarm_snapshot[0] <
 * g_settings.alarm_thres_max)) &&<]*/
/*[>((g_alarm_snapshot[1] > g_settings.alarm_thres_min) &&
 * (g_alarm_snapshot[1] < g_settings.alarm_thres_max)) &&<]*/
/*[>((g_alarm_snapshot[2] > g_settings.alarm_thres_min) &&
 * (g_alarm_snapshot[2] < g_settings.alarm_thres_max))) {<]*/
/*[>// prepare for alarm mode<]*/
/*[>FLAG_ALARM_SETTLE = 0;<]*/
/*[>all_lights_off();<]*/
/*[>g_alarm_counter = g_settings.alarm_counter;<]*/
/*[>g_trigger_counter = 0;<]*/
/*[>g_state = ST_ALARM;<]*/
/*[>}<]*/
/*[>}<]*/
/*}*/

/*// triggers alarm if movement (voltage change) is high enough*/
/*// 1.5g setting:  Sensitivity: 800 mV/g  -1g = 850mV  0g = 1650mV 1g =
 * 2450mV*/
/*// 1 degree tilt is about 9mv, so by default: if the voltage differs 90mv
 * (~10 degrees), for a certain period, the alarm will trigger*/
/*void check_alarm_trigger() {*/
/*[>if (g_state == ST_ALARM) {<]*/
/*[>if (FLAG_ALARM_TRIGGER) {<]*/
/*[>if (FLAG_ALARM_BLINK) {<]*/
/*[>FLAG_ALARM_BLINK = 0;<]*/
/*[>PORT_C90_LIGHT_RI_F ^= (1 << PIN_C90_LIGHT_RI_F);<]*/
/*[>PORT_C90_LIGHT_RI_B ^= (1 << PIN_C90_LIGHT_RI_B);<]*/
/*[>PORT_C90_LIGHT_LI_F ^= (1 << PIN_C90_LIGHT_LI_F);<]*/
/*[>PORT_C90_LIGHT_LI_B ^= (1 << PIN_C90_LIGHT_LI_B);<]*/
/*[>PORT_C90_LIGHT_INDICATOR_COCKPIT ^= (1 <<
 * PIN_C90_LIGHT_INDICATOR_COCKPIT);<]*/
/*[>PORT_C90_CLAXON ^= (1 << PIN_C90_CLAXON);<]*/
/*[>g_alarm_counter--;<]*/
/*[>if (!g_alarm_counter) {<]*/
/*[>all_lights_off();<]*/
/*[>FLAG_ALARM_TRIGGER = 0; // reset alarm trigger<]*/
/*[>g_state = ST_ALARM_SETTLE; // go back to new alarm settle mode<]*/
/*[>FLAG_ALARM_SETTLE = 0; // reset alarm settle<]*/
/*[>}<]*/
/*[>}<]*/
/*[>} else if (FLAG_ALARM_BLINK) {<]*/
/*[>FLAG_ALARM_BLINK = 0;<]*/
/*[>if (g_alarm_blink_counter % 4 == 0) { // blink for 1 tick every timer1 4
 * counts<]*/
/*[>PORT_C90_LIGHT_INDICATOR_COCKPIT |= (1 <<
 * PIN_C90_LIGHT_INDICATOR_COCKPIT);<]*/
/*[>} else {<]*/
/*[>PORT_C90_LIGHT_INDICATOR_COCKPIT &= ~(1 <<
 * PIN_C90_LIGHT_INDICATOR_COCKPIT);<]*/
/*[>}<]*/
/*[>if ((abs(g_adc_voltage[1] - g_alarm_snapshot[0]) >
 * g_settings.alarm_trigger)
 * ||<]*/
/*[>(abs(g_adc_voltage[2] - g_alarm_snapshot[1]) > g_settings.alarm_trigger)
 * ||<]*/
/*[>(abs(g_adc_voltage[3] - g_alarm_snapshot[2]) >
 * g_settings.alarm_trigger))
 * {<]*/
/*[>g_trigger_counter++;<]*/
/*[>if (g_trigger_counter >= g_settings.alarm_trigger_counter) {<]*/
/*[>FLAG_ALARM_TRIGGER = 1;<]*/
/*[>}<]*/
/*[>}<]*/
/*[>}<]*/
/*[>}<]*/
/*}*/

void check_alarm_settle() {}

void check_alarm_trigger() {}

// polling command for SPI intercommunication
// 10hz check
void check_stats() {
  if (g_stats_timer >= 25) {  // 20 ticks x 5ms=100ms interval poll
    g_stats_timer = 5;
    command_trigger_stats(CMD_IF_IC);
#ifdef EASY_TRACE
    command_trigger_stats(CMD_IF_DEBUG);
#endif
  }
}

// check for new GPS info
// 10hz check
void check_gps() {
  if (g_gps_timer >= 35) {  // 20 ticks x 5ms=100ms interval poll, offset is
                            // 50ms to stats trigger to spread a bit
    g_gps_timer = 15;
    command_trigger_gps(CMD_IF_IC);
#ifdef EASY_TRACE
    command_trigger_gps(CMD_IF_DEBUG);
#endif
  }
}

// retrieve new RTC date/time
void check_rtc() {
  if (FLAG_RTC) {
    g_seconds_prev = g_datetime.seconds;
    ds1307_get_date(&g_datetime);  // get current date from RTC
    if (g_seconds_prev !=
        g_datetime.seconds) {  // new second reached on RTC, resync ms timer
      g_milliseconds = 0;
    }
    FLAG_RTC = 0;
  }
}

// gear setting comming from MCU1
void check_neutral() {
  if (!g_gear) {
    set_event(EV_NEUTRAL_ON);
  } else {
    set_event(EV_NEUTRAL_OFF);
  }
}

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
    g_buffer_tail++;  // also move tail, basically destroying the oldest
                      // event to make space
  }
  if (g_buffer_tail >= C90_EVENT_BUFFER_SIZE) {
    g_buffer_tail = 0;  // cycle back to start
  }
}

// default on-switcher for (relay) pins
void process_default_on(volatile uint8_t *port, uint8_t pin,
                        uint16_t substate) {
  *port |= (1 << pin);
  set_substate(substate);
}

// default off-switcher for (relay) pins
void process_default_off(volatile uint8_t *port, uint8_t pin,
                         uint16_t substate) {
  *port &= ~(1 << pin);
  remove_substate(substate);
}

void process_brake_on() {
  process_default_on(&PORT_C90_BRAKE, PIN_C90_BRAKE, ST_BRAKE);
}

void process_brake_off() {
  process_default_off(&PORT_C90_BRAKE, PIN_C90_BRAKE, ST_BRAKE);
}

void process_claxon_on() {
  process_default_on(&PORT_C90_CLAXON, PIN_C90_CLAXON, ST_CLAXON);
}

void process_claxon_off() {
  process_default_off(&PORT_C90_CLAXON, PIN_C90_CLAXON, ST_CLAXON);
}

void process_pilot_on() {
  process_default_on(&PORT_C90_PILOT, PIN_C90_PILOT, ST_PILOT);
}

void process_pilot_off() {
  process_default_off(&PORT_C90_PILOT, PIN_C90_PILOT, ST_PILOT);
}

void process_light_on() {
  process_default_on(&PORT_C90_LIGHT, PIN_C90_LIGHT, ST_LIGHT);
}

void process_light_off() {
  process_default_off(&PORT_C90_LIGHT, PIN_C90_LIGHT, ST_LIGHT);
}

void process_ign_on() {
#ifdef EASY_TRACE
  uart_put_str_1("PROCESS_IGN_ON\r\n");
#endif
  all_relays(FLAG_LIGHT_OFF, FLAG_CLAXON_OFF);
  command_trigger_sound(g_settings.startup_sound, CMD_IF_IC);
#ifdef EASY_TRACE
  command_trigger_sound(g_settings.startup_sound, CMD_IF_DEBUG);
#endif
  set_state(ST_ACTIVE);
}

void process_ign_off() {
#ifdef EASY_TRACE
  uart_put_str_1("PROCESS_IGN_OFF\r\n");
#endif
  all_relays(FLAG_LIGHT_OFF, FLAG_CLAXON_OFF);
  command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
#ifdef EASY_TRACE
  command_trigger_sound(SOUND_CMD_OFF, CMD_IF_DEBUG);
#endif
  if (get_substate(ST_ALARM_SET)) {
    set_state(ST_ALARM);
  } else {
    set_state(ST_SLEEP);
  }
}

void process_neutral_on() {
  set_substate(ST_NEUTRAL);
  PORT_C90_LIGHT_STATUS_COCKPIT |= (1 << PIN_C90_LIGHT_STATUS_COCKPIT);
}

void process_neutral_off() {
  remove_substate(ST_NEUTRAL);
  PORT_C90_LIGHT_STATUS_COCKPIT &= ~(1 << PIN_C90_LIGHT_STATUS_COCKPIT);
}

void process_ri_on() {
  if (FLAG_BLINK_RI) {           // blink indicators
    FLAG_BLINK_RI = 0;           // reset to wait for next timer event
    if (!get_substate(ST_RI)) {  // first time to blink
      TCNT1 = 0;  // set 0.5 sec timer counter explicitly to 0, so the first
                  // blink happens exactly 0.5 secs later
      set_substate(ST_RI);
      PORT_C90_LIGHT_RI_F |= (1 << PIN_C90_LIGHT_RI_F);
      PORT_C90_LIGHT_RI_B |= (1 << PIN_C90_LIGHT_RI_B);
      PORT_C90_LIGHT_INDICATOR_COCKPIT |=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
    } else {
      PORT_C90_LIGHT_RI_F ^= (1 << PIN_C90_LIGHT_RI_F);
      PORT_C90_LIGHT_RI_B ^= (1 << PIN_C90_LIGHT_RI_B);
      PORT_C90_LIGHT_INDICATOR_COCKPIT ^=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
      if (g_settings.indicator_sound) {
        if (PORT_C90_LIGHT_RI_F &
            (1 << PIN_C90_LIGHT_RI_F)) {  // beep when indicators are on
          command_trigger_sound(SOUND_CMD_ON, CMD_IF_IC);
        } else {
          command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
        }
      }
    }
  }
}

void process_ri_off() {
  // disable state/indicators
  remove_substate(ST_RI);
  PORT_C90_LIGHT_RI_F &= ~(1 << PIN_C90_LIGHT_RI_F);  // front
  PORT_C90_LIGHT_RI_B &= ~(1 << PIN_C90_LIGHT_RI_B);  // rear
  PORT_C90_LIGHT_INDICATOR_COCKPIT &=
      ~(1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);  // cockpit
  if (g_settings.indicator_sound) {
    command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
  }
}

void process_li_on() {
  if (FLAG_BLINK_LI) {           // blink indicators
    FLAG_BLINK_LI = 0;           // reset to wait for next timer event
    if (!get_substate(ST_LI)) {  // first time to blink
      TCNT1 = 0;  // set 0.5 sec timer counter explicitly to 0, so the first
                  // blink happens exactly 0.5 secs later
      set_substate(ST_LI);
      PORT_C90_LIGHT_LI_F |= (1 << PIN_C90_LIGHT_LI_F);  // front
      PORT_C90_LIGHT_LI_B |= (1 << PIN_C90_LIGHT_LI_B);  // rear
      PORT_C90_LIGHT_INDICATOR_COCKPIT |=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);  // cockpit
    } else {
      PORT_C90_LIGHT_LI_F ^= (1 << PIN_C90_LIGHT_LI_F);  // front
      PORT_C90_LIGHT_LI_B ^= (1 << PIN_C90_LIGHT_LI_B);  // rear
      PORT_C90_LIGHT_INDICATOR_COCKPIT ^=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);  // cockpit
      if (g_settings.indicator_sound) {
        if (PORT_C90_LIGHT_LI_F &
            (1 << PIN_C90_LIGHT_LI_F)) {  // beep when indicators are on
          command_trigger_sound(SOUND_CMD_ON, CMD_IF_IC);
        } else {
          command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
        }
      }
    }
  }
}

void process_li_off() {
  // disable state/indicators
  remove_substate(ST_LI);
  PORT_C90_LIGHT_LI_F &= ~(1 << PIN_C90_LIGHT_LI_F);  // front
  PORT_C90_LIGHT_LI_B &= ~(1 << PIN_C90_LIGHT_LI_B);  // rear
  PORT_C90_LIGHT_INDICATOR_COCKPIT &=
      ~(1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);  // cockpit
  if (g_settings.indicator_sound) {
    command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
  }
}

void process_warning_on() {
  if (FLAG_BLINK_WARNING) {           // blink indicators
    FLAG_BLINK_WARNING = 0;           // reset to wait for next timer event
    if (!get_substate(ST_WARNING)) {  // first time to blink
      TCNT1 = 0;  // set 0.5 sec timer counter explicitly to 0, so the first
                  // blink happens exactly 0.5 secs later
      // initial ON for indicators to prevent a RI/LI ON messing with the
      // synchronization, i.e. warning lights override indicator switches
      PORT_C90_LIGHT_RI_F |= (1 << PIN_C90_LIGHT_RI_F);
      PORT_C90_LIGHT_RI_B |= (1 << PIN_C90_LIGHT_RI_B);
      PORT_C90_LIGHT_LI_F |= (1 << PIN_C90_LIGHT_LI_F);
      PORT_C90_LIGHT_LI_B |= (1 << PIN_C90_LIGHT_LI_B);
      PORT_C90_LIGHT_INDICATOR_COCKPIT |=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
      set_substate(ST_WARNING);
    } else {
      PORT_C90_LIGHT_RI_F ^= (1 << PIN_C90_LIGHT_RI_F);
      PORT_C90_LIGHT_RI_B ^= (1 << PIN_C90_LIGHT_RI_B);
      PORT_C90_LIGHT_LI_F ^= (1 << PIN_C90_LIGHT_LI_F);
      PORT_C90_LIGHT_LI_B ^= (1 << PIN_C90_LIGHT_LI_B);
      PORT_C90_LIGHT_INDICATOR_COCKPIT ^=
          (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
      if (g_settings.indicator_sound) {
        if (PORT_C90_LIGHT_LI_F &
            (1 << PIN_C90_LIGHT_LI_F)) {  // beep when indicators are on
          command_trigger_sound(SOUND_CMD_ON, CMD_IF_IC);
        } else {
          command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
        }
      }
    }
  }
}

void process_warning_off() {
  // disable state/indicators
  remove_substate(ST_WARNING);
  PORT_C90_LIGHT_LI_F &= ~(1 << PIN_C90_LIGHT_LI_F);  // front
  PORT_C90_LIGHT_LI_B &= ~(1 << PIN_C90_LIGHT_LI_B);  // rear
  PORT_C90_LIGHT_RI_F &= ~(1 << PIN_C90_LIGHT_RI_F);  // front
  PORT_C90_LIGHT_RI_B &= ~(1 << PIN_C90_LIGHT_RI_B);  // rear
  PORT_C90_LIGHT_INDICATOR_COCKPIT &=
      ~(1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);  // cockpit
  if (g_settings.indicator_sound) {
    command_trigger_sound(SOUND_CMD_OFF, CMD_IF_IC);
  }
}

void process_alarm_on() { set_substate(ST_ALARM_SET); }

void process_alarm_off() { remove_substate(ST_ALARM_SET); }

// all relays on/off
void all_relays(uint8_t lights, uint8_t claxon) {
  if (lights) {
    PORT_C90_LIGHT_INDICATOR_COCKPIT |= (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
    PORT_C90_LIGHT_STATUS_COCKPIT |= (1 << PIN_C90_LIGHT_STATUS_COCKPIT);
    PORT_C90_LIGHT_RI_F |= (1 << PIN_C90_LIGHT_RI_F);
    PORT_C90_LIGHT_RI_B |= (1 << PIN_C90_LIGHT_RI_B);
    PORT_C90_LIGHT_LI_F |= (1 << PIN_C90_LIGHT_LI_F);
    PORT_C90_LIGHT_LI_B |= (1 << PIN_C90_LIGHT_LI_B);
    PORT_C90_BRAKE |= (1 << PIN_C90_BRAKE);
    PORT_C90_PILOT |= (1 << PIN_C90_PILOT);
    PORT_C90_LIGHT |= (1 << PIN_C90_LIGHT);
  } else {
    PORT_C90_LIGHT_INDICATOR_COCKPIT &= ~(1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
    PORT_C90_LIGHT_STATUS_COCKPIT &= ~(1 << PIN_C90_LIGHT_STATUS_COCKPIT);
    PORT_C90_LIGHT_RI_F &= ~(1 << PIN_C90_LIGHT_RI_F);
    PORT_C90_LIGHT_RI_B &= ~(1 << PIN_C90_LIGHT_RI_B);
    PORT_C90_LIGHT_LI_F &= ~(1 << PIN_C90_LIGHT_LI_F);
    PORT_C90_LIGHT_LI_B &= ~(1 << PIN_C90_LIGHT_LI_B);
    PORT_C90_BRAKE &= ~(1 << PIN_C90_BRAKE);
    PORT_C90_PILOT &= ~(1 << PIN_C90_PILOT);
    PORT_C90_LIGHT &= ~(1 << PIN_C90_LIGHT);
  }
  if (claxon) {
    PORT_C90_CLAXON |= (1 << PIN_C90_CLAXON);
  } else {
    PORT_C90_CLAXON &= ~(1 << PIN_C90_CLAXON);
  }
}

void init_ports() {
  // sense pins DDR/PORT: as input(0)/high(1)
  DDR_C90_SENSE_BRAKE &= ~(1 << PIN_C90_SENSE_BRAKE);
  PORT_C90_SENSE_BRAKE |= (1 << PIN_C90_SENSE_BRAKE);
  DDR_C90_SENSE_CLAXON &= ~(1 << PIN_C90_SENSE_CLAXON);
  PORT_C90_SENSE_CLAXON |= (1 << PIN_C90_SENSE_CLAXON);
  DDR_C90_SENSE_PILOT &= ~(1 << PIN_C90_SENSE_PILOT);
  PORT_C90_SENSE_PILOT |= (1 << PIN_C90_SENSE_PILOT);
  DDR_C90_SENSE_LIGHT &= ~(1 << PIN_C90_SENSE_LIGHT);
  PORT_C90_SENSE_LIGHT |= (1 << PIN_C90_SENSE_LIGHT);
  DDR_C90_SENSE_IGN &= ~(1 << PIN_C90_SENSE_IGN);
  PORT_C90_SENSE_IGN |= (1 << PIN_C90_SENSE_IGN);
  DDR_C90_SENSE_LIGHT_RI &= ~(1 << PIN_C90_SENSE_LIGHT_RI);
  PORT_C90_SENSE_LIGHT_RI |= (1 << PIN_C90_SENSE_LIGHT_RI);
  DDR_C90_SENSE_LIGHT_LI &= ~(1 << PIN_C90_SENSE_LIGHT_LI);
  PORT_C90_SENSE_LIGHT_LI |= (1 << PIN_C90_SENSE_LIGHT_LI);
  DDR_C90_SENSE_WARNING &= ~(1 << PIN_C90_SENSE_WARNING);
  PORT_C90_SENSE_WARNING |= (1 << PIN_C90_SENSE_WARNING);
  DDR_C90_SENSE_ALARM &= ~(1 << PIN_C90_SENSE_ALARM);
  PORT_C90_SENSE_ALARM |= (1 << PIN_C90_SENSE_ALARM);
  // relay pins DDR/PORT: as output(1)/low(0)
  DDR_C90_BRAKE |= (1 << PIN_C90_BRAKE);
  DDR_C90_CLAXON |= (1 << PIN_C90_CLAXON);
  DDR_C90_PILOT |= (1 << PIN_C90_PILOT);
  DDR_C90_LIGHT |= (1 << PIN_C90_LIGHT);
  DDR_C90_LIGHT_RI_F |= (1 << PIN_C90_LIGHT_RI_F);
  DDR_C90_LIGHT_RI_B |= (1 << PIN_C90_LIGHT_RI_B);
  DDR_C90_LIGHT_LI_F |= (1 << PIN_C90_LIGHT_LI_F);
  DDR_C90_LIGHT_LI_B |= (1 << PIN_C90_LIGHT_LI_B);
  DDR_C90_LIGHT_INDICATOR_COCKPIT |= (1 << PIN_C90_LIGHT_INDICATOR_COCKPIT);
  DDR_C90_LIGHT_STATUS_COCKPIT |= (1 << PIN_C90_LIGHT_STATUS_COCKPIT);
  // heartbeat led on pcb
  DDR_C90_HEARTBEAT_LED |= (1 << PIN_C90_HEARTBEAT_LED);
  PORT_C90_HEARTBEAT_LED |= (1 << PIN_C90_HEARTBEAT_LED);
}

void set_p_senses_active(uint16_t senses) {
  g_settings.p_senses_active = senses;
  update_settings(&g_settings);
}

void set_d_senses_active(uint16_t senses) {
  g_settings.d_senses_active = senses;
  update_settings(&g_settings);
}

void set_d_senses_status(uint16_t senses) { g_d_senses_status = senses; }

// 5ms sense timer for debouncing the buttons
// and for general time tracking like polling intervals
void start_sense_timer() {
  // Configure timer 0 (8-bit) for CTC mode (Clear on Timer Compare)
  // set timer as CTC Mode
  TCCR0A |= (1 << WGM01);
  TCCR0A |= ~(1 << WGM00);
  TCCR0B |= ~(1 << WGM02);
  // prescale 1024
  TCCR0B |= ((1 << CS00) | (1 << CS02));
  TCCR0B &= ~(1 << CS01);
  TIMSK0 |= (1 << OCIE0A);  // Enable CTC interrupt A
  OCR0A = 97;               // Set CTC compare A value, approx. every 5 ms
}

// timer for the blink speed of indicator lights
// NOTE: the TCNT1 counter gets reset in some functions, so dont
// use this blink timer for anything else
void start_blink_timer() {
  // Configure timer 1 (16-bit) for CTC mode (Clear on Timer Compare)
  TCCR1A &= ~((1 << WGM11) | (1 << WGM10));  // CTC Mode
  TCCR1B |= (1 << WGM12);                    // CTC Mode
  // prescale 1024
  TCCR1B |= ((1 << CS10) | (1 << CS12));
  TCCR1B &= ~(1 << CS11);
  TIMSK1 |= (1 << OCIE1A);         // Enable Compare A interrupt
  OCR1A = g_settings.blink_speed;  // Set CTC compare value
}

// timer for the millisecond fraction of global timestamps
void start_ms_timer() {
  // Configure timer 3 (16-bit) for CTC mode (Clear on Timer Compare)
  TCCR3A &= ~((1 << WGM31) | (1 << WGM30));  // CTC Mode
  TCCR3B |= (1 << WGM32);                    // CTC Mode
  // prescale with 8 to get 2.5million hz with a 20mhz crystal
  TCCR3B &= ~((1 << CS30) | (1 << CS32));
  TCCR3B |= (1 << CS31);
  TIMSK3 |= (1 << OCIE3A);  // Enable Compare A interrupt
  OCR3A = 2499;  // Set CTC compare value, every 1ms (counts from 0 to 2499
                 // -> 2.5million hz / 2500 = 1000)
}

// interrupt handler for 8bit timer0 that triggers every 5ms
ISR(TIMER0_COMPA_vect) {
  FLAG_DEBOUNCE_BRAKE = 1;
  FLAG_DEBOUNCE_CLAXON = 1;
  FLAG_DEBOUNCE_IGN = 1;
  FLAG_DEBOUNCE_PILOT = 1;
  FLAG_DEBOUNCE_LIGHT = 1;
  FLAG_DEBOUNCE_RI = 1;
  FLAG_DEBOUNCE_LI = 1;
  FLAG_DEBOUNCE_ALARM = 1;
  FLAG_DEBOUNCE_WARNING = 1;
  g_stats_timer++;
  g_gps_timer++;
  // TODO
  /*if (g_current_alarm_settle_time) {*/
  /*g_current_alarm_settle_time--;*/
  /*}*/
}

// indicator on/off pace, runs every 0.5 secs by default settings
ISR(TIMER1_COMPA_vect) {
  FLAG_BLINK_RI = 1;
  FLAG_BLINK_LI = 1;
  FLAG_BLINK_WARNING = 1;
  // TODO
  /*FLAG_ALARM_BLINK = 1;*/
  /*g_alarm_blink_counter++;*/
}

// timer interrupt for milliseconds counter for general timestamps
ISR(TIMER3_COMPA_vect) {
  g_update_date++;
  // only increment when not about to rollover to prevent out-of-sync
  // discrepancy (e.g. 25.999 to 25.000)
  if (g_milliseconds < 999) {
    g_milliseconds++;
  }
  g_datetime.milliseconds = g_milliseconds;
  if (g_update_date > 39) {  // 25hz polling
    g_update_date = 0;
    FLAG_RTC = 1;  // poll the RTC chip for a new date/time
  }
}

void initialize() {
  g_mcu_reset = 0;
  MCUCR = 0x80;  // disable JTAG at runtime (2 calls in a row needed)
  MCUCR = 0x80;
  wdt_enable(WDTO_2S);                // enable 2 sec watchdog
  sei();                              // enable global interrupts
  g_buffer_head = g_buffer_tail = 0;  // init event buffer
  // get settings from EEPROM
  read_settings(&g_settings);
  // write new power cycle count to EEPROM
  g_settings.power_cycles += 1;
  update_settings(&g_settings);

  // TODO: move to CMD_RTC function
  /*g_datetime.weekday = 4;*/
  /*g_datetime.day = 1;*/
  /*g_datetime.month = 10;*/
  /*g_datetime.year = 15;*/
  /*g_datetime.hours = 22;*/
  /*g_datetime.minutes = 27;*/
  /*g_datetime.seconds = 0;*/
  /*ds1307_set_date(&g_datetime); // set inital date/time*/
  /*_delay_ms(100);*/

  init_ports();
  all_relays(FLAG_LIGHT_OFF, FLAG_CLAXON_OFF);
  start_sense_timer();
  start_blink_timer();
  start_ms_timer();
  command_init();  // init all communication logic, like SPI/UART/I2C buses
  ds1307_get_date(&g_datetime);  // get initial date from RTC
  g_seconds_prev = g_datetime.seconds;
  set_state(ST_SLEEP);  // always start in sleep mode (until IGN_ON fires)
#ifdef EASY_TRACE
  uart_put_str_1("INIT MCU2\r\n");
  uart_put_str_1("SYSTEM NAME: ");
  uart_put_str_1(g_settings.system_name);
  uart_put_str_1("\r\n");
  uart_put_str_1("POWER CYCLES: ");
  uart_put_int_1(g_settings.power_cycles);
  uart_put_str_1("\r\n");
#endif
}

int main(void) {
  initialize();
  while (1) {
    dispatch_events();
    while ((g_event = get_event()) !=
           EV_VOID) {  // handle the complete event queue in one go
      for (uint8_t i = 0; i < TR_CNT; i++) {
        if (g_event == g_trans[i].event) {  // check if event matches
          if (g_trans[i].check_func()) {  // guard condition to check if current
                                          // state is accepting this event
            g_trans[i].process_func();    // call event handler
            break;
          }
        }
      }
    }
    command_process();
    if (!g_mcu_reset) {
      wdt_reset();  // reset the watchdog, i.e. don't reset the mcu
    }
  }
  return 0;  // keep C-compiler happy, although will never reach this
             // point
}

// EOF
