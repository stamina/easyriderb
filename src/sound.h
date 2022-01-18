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
#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

uint16_t calc_note_duration(const uint16_t length, const uint8_t tempo);

#define MUSIC_END 0  // end flag of music
#define MUSIC_P 1    // music pause

#define a0 5682   // PWM: 27.50 Hz, note freq: 27.50 Hz, error 0.00%
#define a0x 5363  // PWM: 29.13 Hz, note freq: 29.14 Hz, error 0.00%
#define b0 5062   // PWM: 30.87 Hz, note freq: 30.87 Hz, error 0.00%
#define c0 9556   // PWM: 16.35 Hz, note freq: 16.35 Hz, error 0.00%
#define c0x 9019  // PWM: 17.32 Hz, note freq: 17.32 Hz, error 0.00%
#define d0 8513   // PWM: 18.35 Hz, note freq: 18.35 Hz, error 0.00%
#define d0x 8035  // PWM: 19.45 Hz, note freq: 19.45 Hz, error 0.00%
#define e0 7584   // PWM: 20.60 Hz, note freq: 20.60 Hz, error 0.00%
#define f0 7159   // PWM: 21.83 Hz, note freq: 21.83 Hz, error 0.00%
#define f0x 6757  // PWM: 23.12 Hz, note freq: 23.12 Hz, error 0.00%
#define g0 6378   // PWM: 24.50 Hz, note freq: 24.50 Hz, error 0.01%
#define g0x 6020  // PWM: 25.96 Hz, note freq: 25.96 Hz, error 0.01%
#define a1 2841   // PWM: 55.00 Hz, note freq: 55.00 Hz, error 0.00%
#define a1x 2681  // PWM: 58.28 Hz, note freq: 58.27 Hz, error 0.02%
#define b1 2531   // PWM: 61.73 Hz, note freq: 61.74 Hz, error 0.00%
#define c1 4778   // PWM: 32.70 Hz, note freq: 32.70 Hz, error 0.00%
#define c1x 4510  // PWM: 34.65 Hz, note freq: 34.65 Hz, error 0.01%
#define d1 4257   // PWM: 36.70 Hz, note freq: 36.71 Hz, error 0.01%
#define d1x 4018  // PWM: 38.89 Hz, note freq: 38.89 Hz, error 0.01%
#define e1 3792   // PWM: 41.21 Hz, note freq: 41.20 Hz, error 0.00%
#define f1 3579   // PWM: 43.66 Hz, note freq: 43.65 Hz, error 0.01%
#define f1x 3378  // PWM: 46.26 Hz, note freq: 46.25 Hz, error 0.01%
#define g1 3189   // PWM: 49.00 Hz, note freq: 49.00 Hz, error 0.01%
#define g1x 3010  // PWM: 51.91 Hz, note freq: 51.91 Hz, error 0.01%
#define a2 1420   // PWM: 110.04 Hz, note freq: 110.00 Hz, error 0.03%
#define a2x 1341  // PWM: 116.52 Hz, note freq: 116.54 Hz, error 0.02%
#define b2 1265   // PWM: 123.52 Hz, note freq: 123.47 Hz, error 0.04%
#define c2 2389   // PWM: 65.40 Hz, note freq: 65.41 Hz, error 0.00%
#define c2x 2255  // PWM: 69.29 Hz, note freq: 69.30 Hz, error 0.01%
#define d2 2128   // PWM: 73.43 Hz, note freq: 73.42 Hz, error 0.01%
#define d2x 2009  // PWM: 77.78 Hz, note freq: 77.78 Hz, error 0.01%
#define e2 1896   // PWM: 82.41 Hz, note freq: 82.41 Hz, error 0.00%
#define f2 1790   // PWM: 87.29 Hz, note freq: 87.31 Hz, error 0.02%
#define f2x 1689  // PWM: 92.51 Hz, note freq: 92.50 Hz, error 0.01%
#define g2 1594   // PWM: 98.02 Hz, note freq: 98.00 Hz, error 0.03%
#define g2x 1505  // PWM: 103.82 Hz, note freq: 103.83 Hz, error 0.01%
#define a3 710    // PWM: 220.07 Hz, note freq: 220.00 Hz, error 0.03%
#define a3x 670   // PWM: 233.21 Hz, note freq: 233.08 Hz, error 0.05%
#define b3 633    // PWM: 246.84 Hz, note freq: 246.94 Hz, error 0.04%
#define c3 1194   // PWM: 130.86 Hz, note freq: 130.81 Hz, error 0.04%
#define c3x 1127  // PWM: 138.64 Hz, note freq: 138.59 Hz, error 0.04%
#define d3 1064   // PWM: 146.85 Hz, note freq: 146.83 Hz, error 0.01%
#define d3x 1004  // PWM: 155.63 Hz, note freq: 155.56 Hz, error 0.04%
#define e3 948    // PWM: 164.82 Hz, note freq: 164.81 Hz, error 0.00%
#define f3 895    // PWM: 174.58 Hz, note freq: 174.61 Hz, error 0.02%
#define f3x 845   // PWM: 184.91 Hz, note freq: 185.00 Hz, error 0.05%
#define g3 797    // PWM: 196.05 Hz, note freq: 196.00 Hz, error 0.03%
#define g3x 752   // PWM: 207.78 Hz, note freq: 207.65 Hz, error 0.06%
#define a4 355    // PWM: 440.14 Hz, note freq: 440.00 Hz, error 0.03%
#define a4x 335   // PWM: 466.42 Hz, note freq: 466.16 Hz, error 0.05%
#define b4 316    // PWM: 494.46 Hz, note freq: 493.88 Hz, error 0.12%
#define c4 597    // PWM: 261.73 Hz, note freq: 261.63 Hz, error 0.04%
#define c4x 564   // PWM: 277.04 Hz, note freq: 277.18 Hz, error 0.05%
#define d4 532    // PWM: 293.70 Hz, note freq: 293.66 Hz, error 0.01%
#define d4x 502   // PWM: 311.25 Hz, note freq: 311.13 Hz, error 0.04%
#define e4 474    // PWM: 329.64 Hz, note freq: 329.63 Hz, error 0.00%
#define f4 447    // PWM: 349.55 Hz, note freq: 349.23 Hz, error 0.09%
#define f4x 422   // PWM: 370.26 Hz, note freq: 369.99 Hz, error 0.07%
#define g4 399    // PWM: 391.60 Hz, note freq: 392.00 Hz, error 0.10%
#define g4x 376   // PWM: 415.56 Hz, note freq: 415.30 Hz, error 0.06%
#define a5 178    // PWM: 877.81 Hz, note freq: 880.00 Hz, error 0.25%
#define a5x 168   // PWM: 930.06 Hz, note freq: 932.33 Hz, error 0.24%
#define b5 158    // PWM: 988.92 Hz, note freq: 987.77 Hz, error 0.12%
#define c5 299    // PWM: 522.58 Hz, note freq: 523.25 Hz, error 0.13%
#define c5x 282   // PWM: 554.08 Hz, note freq: 554.37 Hz, error 0.05%
#define d5 266    // PWM: 587.41 Hz, note freq: 587.33 Hz, error 0.01%
#define d5x 251   // PWM: 622.51 Hz, note freq: 622.25 Hz, error 0.04%
#define e5 237    // PWM: 659.28 Hz, note freq: 659.26 Hz, error 0.00%
#define f5 224    // PWM: 697.54 Hz, note freq: 698.46 Hz, error 0.13%
#define f5x 211   // PWM: 740.52 Hz, note freq: 739.99 Hz, error 0.07%
#define g5 199    // PWM: 785.18 Hz, note freq: 783.99 Hz, error 0.15%
#define g5x 188   // PWM: 831.12 Hz, note freq: 830.61 Hz, error 0.06%
#define a6 89     // PWM: 1755.62 Hz, note freq: 1760.00 Hz, error 0.25%
#define a6x 84    // PWM: 1860.12 Hz, note freq: 1864.66 Hz, error 0.24%
#define b6 79     // PWM: 1977.85 Hz, note freq: 1975.53 Hz, error 0.12%
#define c6 149    // PWM: 1048.66 Hz, note freq: 1046.50 Hz, error 0.21%
#define c6x 141   // PWM: 1108.16 Hz, note freq: 1108.73 Hz, error 0.05%
#define d6 133    // PWM: 1174.81 Hz, note freq: 1174.66 Hz, error 0.01%
#define d6x 126   // PWM: 1240.08 Hz, note freq: 1244.51 Hz, error 0.36%
#define e6 119    // PWM: 1313.03 Hz, note freq: 1318.51 Hz, error 0.42%
#define f6 112    // PWM: 1395.09 Hz, note freq: 1396.91 Hz, error 0.13%
#define f6x 106   // PWM: 1474.06 Hz, note freq: 1479.98 Hz, error 0.40%
#define g6 100    // PWM: 1562.50 Hz, note freq: 1567.98 Hz, error 0.35%
#define g6x 94    // PWM: 1662.23 Hz, note freq: 1661.22 Hz, error 0.06%
#define a7 44     // PWM: 3551.14 Hz, note freq: 3520.00 Hz, error 0.88%
#define a7x 42    // PWM: 3720.24 Hz, note freq: 3729.31 Hz, error 0.24%
#define b7 40     // PWM: 3906.25 Hz, note freq: 3951.07 Hz, error 1.15%
#define c7 75     // PWM: 2083.33 Hz, note freq: 2093.00 Hz, error 0.46%
#define c7x 70    // PWM: 2232.14 Hz, note freq: 2217.46 Hz, error 0.66%
#define d7 67     // PWM: 2332.09 Hz, note freq: 2349.32 Hz, error 0.74%
#define d7x 63    // PWM: 2480.16 Hz, note freq: 2489.02 Hz, error 0.36%
#define e7 59     // PWM: 2648.31 Hz, note freq: 2637.02 Hz, error 0.43%
#define f7 56     // PWM: 2790.18 Hz, note freq: 2793.83 Hz, error 0.13%
#define f7x 53    // PWM: 2948.11 Hz, note freq: 2959.96 Hz, error 0.40%
#define g7 50     // PWM: 3125.00 Hz, note freq: 3135.96 Hz, error 0.35%
#define g7x 47    // PWM: 3324.47 Hz, note freq: 3322.44 Hz, error 0.06%
#define a8 22     // PWM: 7102.27 Hz, note freq: 7040.00 Hz, error 0.88%
#define a8x 21    // PWM: 7440.48 Hz, note freq: 7458.62 Hz, error 0.24%
#define b8 20     // PWM: 7812.50 Hz, note freq: 7902.13 Hz, error 1.15%
#define c8 37     // PWM: 4222.97 Hz, note freq: 4186.01 Hz, error 0.88%
#define c8x 35    // PWM: 4464.29 Hz, note freq: 4434.92 Hz, error 0.66%
#define d8 33     // PWM: 4734.85 Hz, note freq: 4698.64 Hz, error 0.76%
#define d8x 31    // PWM: 5040.32 Hz, note freq: 4978.03 Hz, error 1.24%
#define e8 30     // PWM: 5208.33 Hz, note freq: 5274.04 Hz, error 1.26%
#define f8 28     // PWM: 5580.36 Hz, note freq: 5587.65 Hz, error 0.13%
#define f8x 26    // PWM: 6009.62 Hz, note freq: 5919.91 Hz, error 1.49%
#define g8 25     // PWM: 6250.00 Hz, note freq: 6271.93 Hz, error 0.35%
#define g8x 24    // PWM: 6510.42 Hz, note freq: 6644.88 Hz, error 2.07%

// Music Time Signature:
// 4   < the numerator shows how many beats are in the measure (not important
// for the buzzer sound) / - / 4   < the denominator shows what type of note
// gets the accent on the beat: full, half, quarter, eight, sixteenth
static const uint8_t g_music_denominator = 4;
static volatile uint16_t g_music_duration;

#endif
