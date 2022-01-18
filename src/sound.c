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

#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>

#include "sound.h"

#include "sound/alarm.h"
#include "sound/frogger.h"
#include "sound/furelise.h"
#include "sound/larry.h"
#include "sound/pipi.h"
#include "sound/popcorn.h"

// duration of note based on the 5ms timer counter
// 200x 5ms ticks * 60sec/tempo in bmp * (beat note type / length)
// length is normally 1, 2, 4, 8 or 16 parts of a full note
uint16_t calc_note_duration(const uint16_t length, const uint8_t tempo) {
  return 200 * 60 / tempo * g_music_denominator / length;
}
