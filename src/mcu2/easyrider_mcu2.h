/*
 *
 *  Copyright (C) Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
#ifndef EASYRIDER_MCU2_H_INCLUDED
#define EASYRIDER_MCU2_H_INCLUDED

#include "command.h"
#include "ds1307.h"
#include "settings.h"

#define STATUS_C90_SENSE_BRAKE PIND &(1 << PIN_C90_SENSE_BRAKE)
#define STATUS_C90_SENSE_CLAXON PIND &(1 << PIN_C90_SENSE_CLAXON)
#define STATUS_C90_SENSE_PILOT PINB &(1 << PIN_C90_SENSE_PILOT)
#define STATUS_C90_SENSE_LIGHT PINB &(1 << PIN_C90_SENSE_LIGHT)
#define STATUS_C90_SENSE_IGN PIND &(1 << PIN_C90_SENSE_IGN)
#define STATUS_C90_SENSE_LIGHT_RI PINB &(1 << PIN_C90_SENSE_LIGHT_RI)
#define STATUS_C90_SENSE_LIGHT_LI PINB &(1 << PIN_C90_SENSE_LIGHT_LI)
#define STATUS_C90_SENSE_WARNING PIND &(1 << PIN_C90_SENSE_WARNING)
#define STATUS_C90_SENSE_ALARM PINB &(1 << PIN_C90_SENSE_ALARM)

#define PIN_C90_SENSE_BRAKE PIND4
#define PIN_C90_SENSE_CLAXON PIND5
#define PIN_C90_SENSE_PILOT PINB2
#define PIN_C90_SENSE_LIGHT PINB3
#define PIN_C90_SENSE_IGN PIND6
#define PIN_C90_SENSE_LIGHT_RI PINB1
#define PIN_C90_SENSE_LIGHT_LI PINB4
#define PIN_C90_SENSE_WARNING PIND7
#define PIN_C90_SENSE_ALARM PINB0

#define PORT_C90_SENSE_BRAKE PORTD
#define PORT_C90_SENSE_CLAXON PORTD
#define PORT_C90_SENSE_PILOT PORTB
#define PORT_C90_SENSE_LIGHT PORTB
#define PORT_C90_SENSE_IGN PORTD
#define PORT_C90_SENSE_LIGHT_RI PORTB
#define PORT_C90_SENSE_LIGHT_LI PORTB
#define PORT_C90_SENSE_WARNING PORTD
#define PORT_C90_SENSE_ALARM PORTB

#define DDR_C90_SENSE_BRAKE DDRD
#define DDR_C90_SENSE_CLAXON DDRD
#define DDR_C90_SENSE_PILOT DDRB
#define DDR_C90_SENSE_LIGHT DDRB
#define DDR_C90_SENSE_IGN DDRD
#define DDR_C90_SENSE_LIGHT_RI DDRB
#define DDR_C90_SENSE_LIGHT_LI DDRB
#define DDR_C90_SENSE_WARNING DDRD
#define DDR_C90_SENSE_ALARM DDRB

#define DDR_C90_BRAKE DDRA
#define DDR_C90_CLAXON DDRA
#define DDR_C90_PILOT DDRA
#define DDR_C90_LIGHT DDRC
#define DDR_C90_LIGHT_RI_F DDRA
#define DDR_C90_LIGHT_RI_B DDRC
#define DDR_C90_LIGHT_LI_F DDRC
#define DDR_C90_LIGHT_LI_B DDRC
#define DDR_C90_LIGHT_INDICATOR_COCKPIT DDRA
#define DDR_C90_LIGHT_STATUS_COCKPIT DDRC
#define DDR_C90_HEARTBEAT_LED DDRC

#define PORT_C90_BRAKE PORTA
#define PORT_C90_CLAXON PORTA
#define PORT_C90_PILOT PORTA
#define PORT_C90_LIGHT PORTC
#define PORT_C90_LIGHT_RI_F PORTA
#define PORT_C90_LIGHT_LI_F PORTC
#define PORT_C90_LIGHT_RI_B PORTC
#define PORT_C90_LIGHT_LI_B PORTC
#define PORT_C90_LIGHT_INDICATOR_COCKPIT PORTA
#define PORT_C90_LIGHT_STATUS_COCKPIT PORTC
#define PORT_C90_HEARTBEAT_LED PORTC

#define PIN_C90_BRAKE PINA6
#define PIN_C90_CLAXON PINA5
#define PIN_C90_PILOT PINA3
#define PIN_C90_LIGHT PINC5
#define PIN_C90_LIGHT_RI_F PINA4
#define PIN_C90_LIGHT_LI_F PINC3
#define PIN_C90_LIGHT_RI_B PINC7
#define PIN_C90_LIGHT_LI_B PINC6
#define PIN_C90_LIGHT_INDICATOR_COCKPIT PINA7
#define PIN_C90_LIGHT_STATUS_COCKPIT PINC4
#define PIN_C90_HEARTBEAT_LED PINC2

// the events
#define EV_VOID 0
#define EV_ANY 1
#define EV_RI_ON 2
#define EV_LI_ON 3
#define EV_RI_OFF 4
#define EV_LI_OFF 5
#define EV_CLAXON_ON 6
#define EV_CLAXON_OFF 7
#define EV_BRAKE_ON 8
#define EV_BRAKE_OFF 9
#define EV_PILOT_ON 10
#define EV_PILOT_OFF 11
#define EV_LIGHT_ON 12
#define EV_LIGHT_OFF 13
#define EV_ALARM_ON 14
#define EV_ALARM_OFF 15
#define EV_IGN_ON 16
#define EV_IGN_OFF 17
#define EV_WARNING_ON 18
#define EV_WARNING_OFF 19
#define EV_NEUTRAL_ON 20
#define EV_NEUTRAL_OFF 21

// all possible substate bits that are contained in the full g_state var
// byte 1
#define ST_SLEEP 1    // default state after initial power on
#define ST_ACTIVE 2   // ignition turned on
#define ST_ALARM 4    // alarm mode turned on
#define ST_NEUTRAL 8  // in neutral, no gear selected
#define ST_RI 16      // right indicator on
#define ST_LI 32      // left indicator on
#define ST_CLAXON 64  // claxon on
#define ST_BRAKE 128  // brake on
// byte 2
#define ST_LIGHT 256       // main light on
#define ST_WARNING 512     // warning lights on (4 indicators)
#define ST_PILOT 1024      // pilot front light on
#define ST_ALARM_SET 2048  // alarm switch toggled on

#define FLAG_SENSE_BRAKE 1
#define FLAG_SENSE_CLAXON 2
#define FLAG_SENSE_PILOT 4
#define FLAG_SENSE_LIGHT 8
#define FLAG_SENSE_IGN 16
#define FLAG_SENSE_LIGHT_RI 32
#define FLAG_SENSE_LIGHT_LI 64
#define FLAG_SENSE_WARNING 128
#define FLAG_SENSE_ALARM 256

#define FLAG_LIGHT_ON 1
#define FLAG_LIGHT_OFF 0
#define FLAG_CLAXON_ON 1
#define FLAG_CLAXON_OFF 0

#define SOUND_CMD_ON 253
#define SOUND_CMD_OFF 254

#define TR_CNT \
  (sizeof(g_trans) / sizeof(*g_trans))  // number of transition table entries

// size of event queue (circular buffer) -> this is the max size, if more events
// occur in one go, then the oldest will be overwritten, just like Snake hitting
// his tail
#define C90_EVENT_BUFFER_SIZE 128

// state transition struct
typedef struct {
  uint8_t event;                // event id
  uint8_t (*check_func)(void);  // guard condition function: is current state
                                // allowed to process event?
  void (*process_func)(void);   // process function
} tTransition;

RTCDate g_datetime;    // date/time from RTC
tSettings g_settings;  // active settings (initially read from EEPROM)

//// proto's
static void initialize(void);
static void dispatch_events(void);
static uint8_t get_event(void);
static void set_event(uint8_t ev);
static void start_sense_timer(void);
static void start_blink_timer(void);
static void start_ms_timer(void);
static void init_ports(void);
static void set_state(uint16_t st);
static void set_substate(uint16_t st);
static void remove_substate(uint16_t st);
static uint16_t get_substate(uint16_t st);
static void process_default_on(volatile uint8_t *port, uint8_t pin,
                               uint16_t substate);
static void process_default_off(volatile uint8_t *port, uint8_t pin,
                                uint16_t substate);
static void process_brake_on(void);
static void process_brake_off(void);
static void process_claxon_on(void);
static void process_claxon_off(void);
static void process_pilot_on(void);
static void process_pilot_off(void);
static void process_light_on(void);
static void process_light_off(void);
static void process_ign_on(void);
static void process_ign_off(void);
static void process_ri_on(void);
static void process_ri_off(void);
static void process_li_on(void);
static void process_li_off(void);
static void process_neutral_on(void);
static void process_neutral_off(void);
static void process_warning_off(void);
static void process_warning_on(void);
static void process_alarm_on(void);
static void process_alarm_off(void);
static void check_default(uint16_t physical_flag, uint8_t event_on,
                          uint8_t event_off, uint8_t pin,
                          volatile uint8_t *debounce_timer_flag,
                          volatile uint8_t *debounce_state);
static uint8_t check_brake_on(void);
static uint8_t check_brake_off(void);
static uint8_t check_claxon_on(void);
static uint8_t check_claxon_off(void);
static uint8_t check_pilot_on(void);
static uint8_t check_pilot_off(void);
static uint8_t check_light_on(void);
static uint8_t check_light_off(void);
static uint8_t check_ign_on(void);
static uint8_t check_ign_off(void);
static uint8_t check_ri_on(void);
static uint8_t check_li_on(void);
static uint8_t check_ri_off(void);
static uint8_t check_li_off(void);
static uint8_t check_warning_on(void);
static uint8_t check_warning_off(void);
static uint8_t check_alarm_on(void);
static uint8_t check_alarm_off(void);
static uint8_t check_neutral_on(void);
static uint8_t check_neutral_off(void);
static void check_alarm(void);
static void check_alarm_settle(void);
static void check_brake(void);
static void check_claxon(void);
static void check_pilot(void);
static void check_light(void);
static void check_ign(void);
static void check_ri(void);
static void check_li(void);
static void check_neutral(void);
static void check_alarm_trigger(void);
static void check_warning(void);
static void check_stats(void);
static void check_gps(void);
static void check_rtc(void);
static void all_relays(uint8_t lights, uint8_t claxon);

void set_p_senses_active(uint16_t senses);
void set_d_senses_active(uint16_t senses);
void set_d_senses_status(uint16_t senses);

// flags
static volatile uint8_t FLAG_BLINK_RI;       // right indicator blink needed
static volatile uint8_t FLAG_BLINK_LI;       // left indicator blink needed
static volatile uint8_t FLAG_BLINK_WARNING;  // all indicators blink needed
static volatile uint8_t FLAG_RTC;  // flag to retrieve a new date/time from RTC
static volatile uint8_t FLAG_DEBOUNCE_ALARM;    // alarm debounce
static volatile uint8_t FLAG_DEBOUNCE_BRAKE;    // brake debounce
static volatile uint8_t FLAG_DEBOUNCE_CLAXON;   // claxon debounce
static volatile uint8_t FLAG_DEBOUNCE_PILOT;    // pilot debounce
static volatile uint8_t FLAG_DEBOUNCE_LIGHT;    // light debounce
static volatile uint8_t FLAG_DEBOUNCE_IGN;      // ignition key debounce
static volatile uint8_t FLAG_DEBOUNCE_RI;       // right indicator debounce
static volatile uint8_t FLAG_DEBOUNCE_LI;       // left indicator debounce
static volatile uint8_t FLAG_DEBOUNCE_WARNING;  // warning debounce

// globals, non-static ones are used in other modules as an "extern", e.g. in
// the command module
volatile uint16_t g_state;            // the current state
volatile uint8_t g_gear;              // current gear or neutral
volatile uint16_t g_accelx;           // X-axis voltage of accelerometer
volatile uint16_t g_accely;           // Y-axis voltage of accelerometer
volatile uint16_t g_accelz;           // Z-axis voltage of accelerometer
volatile uint8_t g_connected;         // authorized connection
volatile uint16_t g_d_senses_status;  // dynamic senses current on/off status
volatile uint8_t g_mcu_reset;  // let the mcu reset by the watchdog, can be
                               // triggered remotely via command

static volatile uint8_t g_event;  // the current event
static volatile uint8_t
    g_event_buffer[C90_EVENT_BUFFER_SIZE];  // the event queue (circular buffer)
static volatile uint8_t g_stats_timer;      // SPI polling command counter
static volatile uint8_t g_gps_timer;        // GPS polling counter
static volatile uint8_t g_buffer_head;      // index of first item to process
static volatile uint8_t g_buffer_tail;      // index of last item to process
static volatile uint16_t
    g_milliseconds;  // current milliseconds for timestamping
static volatile uint8_t g_seconds_prev;  // previous seconds for timestamping,
                                         // needed to reset/sync with ms timer
static volatile uint8_t g_update_date;   // retrieve new date/time from RTC

// debounce bitmasks
static volatile uint8_t g_brake_debounce;    // brake light
static volatile uint8_t g_claxon_debounce;   // claxon sound
static volatile uint8_t g_ign_debounce;      // ignition sound
static volatile uint8_t g_pilot_debounce;    // pilot light
static volatile uint8_t g_light_debounce;    // main light
static volatile uint8_t g_ri_debounce;       // right indicators light
static volatile uint8_t g_li_debounce;       // left indicators light
static volatile uint8_t g_warning_debounce;  // warning switch
static volatile uint8_t g_alarm_debounce;    // alarm switch

// TODO
// static volatile uint16_t g_adc_avg_voltage[3]; // average ADC voltages: 0-2
// is xyz accelerometer static volatile uint8_t g_alarm_blink_counter; //
// counter to determine alarm indicator blinking interval static volatile
// uint8_t g_alarm_counter; // counter to determine the amount of alarm signals
// in a row static volatile uint16_t g_alarm_snapshot[3]; // alarm snapshot
// array which contains the 3-axis' voltages static volatile uint8_t
// g_trigger_counter; // number of positive triggers needed to sound the alarm

#endif
