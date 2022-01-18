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
#ifndef EASYRIDER_MCU1_H_INCLUDED
#define EASYRIDER_MCU1_H_INCLUDED

#define STATUS_C90_SENSE_G1 PIND &(1 << PIN_C90_SENSE_G1)
#define STATUS_C90_SENSE_G2 PINB &(1 << PIN_C90_SENSE_G2)
#define STATUS_C90_SENSE_G3 PINB &(1 << PIN_C90_SENSE_G3)
#define STATUS_C90_SENSE_G4 PINA &(1 << PIN_C90_SENSE_G4)

#define PIN_C90_SENSE_G1 PIND7
#define PIN_C90_SENSE_G2 PINB1
#define PIN_C90_SENSE_G3 PINB2
#define PIN_C90_SENSE_G4 PINA6

#define PORT_C90_SENSE_G1 PORTD
#define PORT_C90_SENSE_G2 PORTB
#define PORT_C90_SENSE_G3 PORTB
#define PORT_C90_SENSE_G4 PORTA

#define DDR_C90_SENSE_G1 DDRD
#define DDR_C90_SENSE_G2 DDRB
#define DDR_C90_SENSE_G3 DDRB
#define DDR_C90_SENSE_G4 DDRA
#define DDR_C90_RPM DDRB
#define DDR_C90_HEARTBEAT_LED DDRC
#define DDR_C90_BUZZER DDRB

#define PORT_C90_RPM PORTB
#define PORT_C90_HEARTBEAT_LED PORTC
#define PORT_C90_BUZZER PORTB

#define PIN_C90_RPM PINB0
#define PIN_C90_HEARTBEAT_LED PINC2
#define PIN_C90_BUZZER PINB3

// the events
#define EV_VOID 0
#define EV_ANY 1
#define EV_READ_BATTERY 2
#define EV_READ_CURRENT 3
#define EV_READ_TEMPERATURE 4
#define EV_READ_ACCEL 5
#define EV_G1_ON 6
#define EV_G1_OFF 7
#define EV_G2_ON 8
#define EV_G2_OFF 9
#define EV_G3_ON 10
#define EV_G3_OFF 11
#define EV_G4_ON 12
#define EV_G4_OFF 13

#define TR_CNT           \
  (sizeof(g_callbacks) / \
   sizeof(*g_callbacks))  // number of callback table entries

// size of event queue (circular buffer) -> this is the max size, if more events
// occur in one go, then the oldest will be overwritten, just like Snake hitting
// his tail
#define C90_EVENT_BUFFER_SIZE 128
#define C90_OFFSET_ADC_READING \
  0  // tweak this if your 5V VRef is a little off, or to compensate an offset
     // error at 0 volt; each in-/decrement is 5/1024th of a volt

// const char g_logo1[] PROGMEM = "    _";
// const char g_logo2[] PROGMEM = ".-.-.=\x5C-.";
// const char g_logo3[] PROGMEM = "(_)=='(_)";
// const char g_logo4[] PROGMEM = "2013 Bas Brugman - http://www.visionnaire.nl
// - type help for command overview"; const char* const g_logo[] PROGMEM = {
// g_logo1, g_logo2, g_logo3, g_logo4
//};
// char *g_appname = "EasyRider";
// char *g_user = "user";
// const char g_firmware_version[] PROGMEM = "EasyRider version 1.0 March 2013";

static volatile uint16_t g_music_duration;
static uint8_t g_music_tempo;

extern const uint16_t g_music_alarm[] PROGMEM;
extern const uint16_t g_music_pipi[] PROGMEM;
extern const uint16_t g_music_popcorn[] PROGMEM;
extern const uint16_t g_music_frogger[] PROGMEM;
extern const uint16_t g_music_larry[] PROGMEM;
extern const uint16_t g_music_furelise[] PROGMEM;

#define SONG_COUNT 5  // number of songs, idx 0 is for the alarm only

static const uint16_t *const g_music[] PROGMEM = {
    g_music_alarm, g_music_pipi,    g_music_popcorn,
    g_music_larry, g_music_frogger, g_music_furelise};

// callback struct
typedef struct {
  uint8_t event;                // event id
  uint8_t (*check_func)(void);  // guard condition function: is current state
                                // allowed to process event?
  void (*process_func)(void);   // process function
} tCallBack;

//// proto's
static void initialize(void);
static void init_ports(void);
static void dispatch_events(void);
static uint8_t get_event(void);
static void set_event(uint8_t ev);
static void set_gear(uint8_t gear);
static void start_rpm_timer(void);
static void start_buzzer_timer(void);
static void start_sense_timer(void);
static void start_10us_timer(void);
static void check_battery(void);
static void check_current(void);
static void check_temperature(void);
static void check_accel(void);
static void check_gear1(void);
static void check_gear2(void);
static void check_gear3(void);
static void check_gear4(void);
static void check_neutral(void);
static uint8_t check_battery_read(void);
static uint8_t check_temperature_read(void);
static uint8_t check_board_current(void);
static uint8_t check_board_accel(void);
static uint8_t check_gear1_on(void);
static uint8_t check_gear2_on(void);
static uint8_t check_gear3_on(void);
static uint8_t check_gear4_on(void);
static uint8_t check_gear1_off(void);
static uint8_t check_gear2_off(void);
static uint8_t check_gear3_off(void);
static uint8_t check_gear4_off(void);
static void process_battery(void);
static void process_temperature(void);
static void process_board_current(void);
static void process_board_accel(void);
static void process_gear1_on(void);
static void process_gear2_on(void);
static void process_gear3_on(void);
static void process_gear4_on(void);
static void process_gear1_off(void);
static void process_gear2_off(void);
static void process_gear3_off(void);
static void process_gear4_off(void);

static void check_sound(void);
static void enable_adc(void);

void set_sound(uint8_t status);

// flags
static volatile uint8_t FLAG_DEBOUNCE_G1;  // gear 1 debounce
static volatile uint8_t FLAG_DEBOUNCE_G2;  // gear 2 debounce
static volatile uint8_t FLAG_DEBOUNCE_G3;  // gear 3 debounce
static volatile uint8_t FLAG_DEBOUNCE_G4;  // gear 4 debounce
static volatile uint8_t
    FLAG_READ_BATTERY;  // ADC voltage readout/conversion needed
static volatile uint8_t
    FLAG_READ_CURRENT;  // ADC voltage readout/conversion needed
static volatile uint8_t
    FLAG_READ_TEMPERATURE;  // ADC voltage readout/conversion needed
static volatile uint8_t
    FLAG_READ_ACCEL;                 // ADC voltage readout/conversion needed
static volatile uint8_t FLAG_MUSIC;  // time to play some music

// globals, non-static ones are used in other modules as an "extern", e.g. in
// the command module
static volatile uint8_t g_event;  // the current event
static volatile uint8_t
    g_event_buffer[C90_EVENT_BUFFER_SIZE];  // the event queue (circular buffer)
static volatile uint8_t g_buffer_head;      // index of first item to process
static volatile uint8_t g_buffer_tail;      // index of last item to process
static volatile uint16_t g_10us_ticks;      // timer ticks every 10 microseconds
static volatile uint8_t g_rpm_reset;     // counter to check if the engine has
                                         // stopped, i.e. reset rpm to 0
static volatile uint8_t g_adc_read_pin;  // current pin to read ADC voltage
static volatile uint16_t
    g_adc_voltage[6];  // current ADC voltages: accelx, accely, accelz, vbat,
                       // vcurrent, vtemp
static volatile uint8_t
    g_current_gear;  // currently selected gear for temporary use to satisfy
                     // on/off gear checks
static volatile uint8_t
    g_neutral_counter;  // x ticks required until definitely in neutral

volatile uint8_t g_gear;       // currently selected gear
volatile uint8_t g_mcu_reset;  // let the mcu reset by the watchdog, can be
                               // triggered remotely via command

// 7 values for statistics
volatile uint16_t g_current;      // board current consumption in mA
volatile uint16_t g_rpm;          // current RPM of engine
volatile uint16_t g_voltage;      // battery voltage
volatile uint16_t g_temperature;  // board ambient temperature
volatile uint16_t g_accelx;       // X-axis voltage of accelerometer
volatile uint16_t g_accely;       // Y-axis voltage of accelerometer
volatile uint16_t g_accelz;       // Z-axis voltage of accelerometer

static uint16_t const *g_selected_music;  // pointer to current song
static uint16_t const
    *g_current_music;  // pointer advancing within current song

#ifdef EASY_TRACE123
static volatile uint8_t
    g_uart_display_counter;  // counter to determine when to display information
                             // over uart1 (DEBUG)
#endif

// debounce bitmasks
static volatile uint8_t g_gear1_debounce;  // gear 1 sense
static volatile uint8_t g_gear2_debounce;  // gear 2 sense
static volatile uint8_t g_gear3_debounce;  // gear 3 sense
static volatile uint8_t g_gear4_debounce;  // gear 4 sense

#endif
