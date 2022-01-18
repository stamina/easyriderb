/*
 *
 *  Copyright (C) Bas Brugman
 *  http://www.visionnaire.nl
 *
 */
#include "settings.h"

// setting these to defaults, so an .eep ROM file is generated
// this struct will be used as the reference layout to read/write to the proper
// pointer addresses in EEPROM
tSettings EEMEM g_rom_settings = {
    ROM_SYSTEM_NAME,       ROM_POWER_CYCLES,
    ROM_PINCODE,           ROM_SD_LOG,
    ROM_SW_VERSION,        ROM_HW_VERSION,
    ROM_P_SENSES_ACTIVE,   ROM_D_SENSES_ACTIVE,
    ROM_ALARM_SETTLE_TIME, ROM_INDICATOR_SOUND,
    ROM_BLINK_SPEED,       ROM_ALARM_COUNTER,
    ROM_ALARM_TRIGGER,     ROM_ALARM_TRIGGER_COUNTER,
    ROM_ALARM_THRES_MIN,   ROM_ALARM_TRHES_MAX,
    ROM_STARTUP_SOUND};

// reads the current settings, or resets it to default settings when no valid
// ROM values are found
void read_settings(tSettings *settings) {
  eeprom_read_block((void *)settings->system_name,
                    (const void *)g_rom_settings.system_name, 10);
  settings->system_name[9] = '\0';  // force valid string in case ROM is invalid
  if (strncmp(settings->system_name, ROM_SYSTEM_NAME, 9) !=
      0) {  // invalid ROM, write default settings first
    reset_settings();
    eeprom_read_block((void *)settings->system_name,
                      (const void *)g_rom_settings.system_name, 10);
  }
  settings->power_cycles = eeprom_read_word(&g_rom_settings.power_cycles);
  eeprom_read_block((void *)settings->pincode,
                    (const void *)g_rom_settings.pincode, 5);
  settings->sd_log = eeprom_read_byte(&g_rom_settings.sd_log);
  settings->sw_version = eeprom_read_word(&g_rom_settings.sw_version);
  eeprom_read_block((void *)settings->hw_version,
                    (const void *)g_rom_settings.hw_version, 10);
  settings->p_senses_active = eeprom_read_word(&g_rom_settings.p_senses_active);
  settings->d_senses_active = eeprom_read_word(&g_rom_settings.d_senses_active);
  settings->alarm_settle_time =
      eeprom_read_word(&g_rom_settings.alarm_settle_time);
  settings->indicator_sound = eeprom_read_byte(&g_rom_settings.indicator_sound);
  settings->blink_speed = eeprom_read_word(&g_rom_settings.blink_speed);
  settings->alarm_counter = eeprom_read_byte(&g_rom_settings.alarm_counter);
  settings->alarm_trigger = eeprom_read_byte(&g_rom_settings.alarm_trigger);
  settings->alarm_trigger_counter =
      eeprom_read_byte(&g_rom_settings.alarm_trigger_counter);
  settings->alarm_thres_min = eeprom_read_word(&g_rom_settings.alarm_thres_min);
  settings->alarm_thres_max = eeprom_read_word(&g_rom_settings.alarm_thres_max);
  settings->startup_sound = eeprom_read_byte(&g_rom_settings.startup_sound);
}

// eeprom_update writes only the changed values to ROM
void update_settings(tSettings *settings) {
  eeprom_update_block((const void *)settings->system_name,
                      &g_rom_settings.system_name, 10);
  eeprom_update_word(&g_rom_settings.power_cycles, settings->power_cycles);
  eeprom_update_block((const void *)settings->pincode, &g_rom_settings.pincode,
                      5);
  eeprom_update_byte(&g_rom_settings.sd_log, settings->sd_log);
  eeprom_update_word(&g_rom_settings.sw_version, settings->sw_version);
  eeprom_update_block((const void *)settings->hw_version,
                      &g_rom_settings.hw_version, 10);
  eeprom_update_word(&g_rom_settings.p_senses_active,
                     settings->p_senses_active);
  eeprom_update_word(&g_rom_settings.d_senses_active,
                     settings->d_senses_active);
  eeprom_update_word(&g_rom_settings.alarm_settle_time,
                     settings->alarm_settle_time);
  eeprom_update_byte(&g_rom_settings.indicator_sound,
                     settings->indicator_sound);
  eeprom_update_word(&g_rom_settings.blink_speed, settings->blink_speed);
  eeprom_update_byte(&g_rom_settings.alarm_counter, settings->alarm_counter);
  eeprom_update_byte(&g_rom_settings.alarm_trigger, settings->alarm_trigger);
  eeprom_update_byte(&g_rom_settings.alarm_trigger_counter,
                     settings->alarm_trigger_counter);
  eeprom_update_word(&g_rom_settings.alarm_thres_min,
                     settings->alarm_thres_min);
  eeprom_update_word(&g_rom_settings.alarm_thres_max,
                     settings->alarm_thres_max);
  eeprom_update_byte(&g_rom_settings.startup_sound, settings->startup_sound);
}

// resets the whole ROM to the default values
void reset_settings() {
  eeprom_update_block(ROM_SYSTEM_NAME, &g_rom_settings.system_name, 10);
  eeprom_update_word(&g_rom_settings.power_cycles, ROM_POWER_CYCLES);
  eeprom_update_block(ROM_PINCODE, &g_rom_settings.pincode, 5);
  eeprom_update_byte(&g_rom_settings.sd_log, ROM_SD_LOG);
  eeprom_update_word(&g_rom_settings.sw_version, ROM_SW_VERSION);
  eeprom_update_block(ROM_HW_VERSION, &g_rom_settings.hw_version, 10);
  eeprom_update_word(&g_rom_settings.p_senses_active, ROM_P_SENSES_ACTIVE);
  eeprom_update_word(&g_rom_settings.d_senses_active, ROM_D_SENSES_ACTIVE);
  eeprom_update_word(&g_rom_settings.alarm_settle_time, ROM_ALARM_SETTLE_TIME);
  eeprom_update_byte(&g_rom_settings.indicator_sound, ROM_INDICATOR_SOUND);
  eeprom_update_word(&g_rom_settings.blink_speed, ROM_BLINK_SPEED);
  eeprom_update_byte(&g_rom_settings.alarm_counter, ROM_ALARM_COUNTER);
  eeprom_update_byte(&g_rom_settings.alarm_trigger, ROM_ALARM_TRIGGER);
  eeprom_update_byte(&g_rom_settings.alarm_trigger_counter,
                     ROM_ALARM_TRIGGER_COUNTER);
  eeprom_update_word(&g_rom_settings.alarm_thres_min, ROM_ALARM_THRES_MIN);
  eeprom_update_word(&g_rom_settings.alarm_thres_max, ROM_ALARM_TRHES_MAX);
  eeprom_update_byte(&g_rom_settings.startup_sound, ROM_STARTUP_SOUND);
}
