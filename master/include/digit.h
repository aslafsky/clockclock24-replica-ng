#ifndef digit_h
#define digit_h

#include <Arduino.h>
#include "clock_state.h"

/**
 * Digits
 * structure: {
 * h0, m0,
 * h1, m1,
 * ...
 * h5, m5
 * }
*/

const t_digit digit_0 = {
  270, 0,
  270, 90,
  0, 90,
  270, 180,
  270, 90,
  180, 90
};

const t_digit digit_1 = {
  225, 225,
  225, 225,
  225, 225,
  270, 270,
  270, 90,
  90, 90
};

const t_digit digit_2 = {
  0, 0,
  270, 0,
  90, 0,
  180, 270,
  90, 180,
  180, 180
};

const t_digit digit_3 = {
  0, 0,
  0, 0,
  0, 0,
  180, 270,
  180, 90,
  180, 90
};

const t_digit digit_4 = {
  270, 270,
  90, 0,
  225, 225,
  270, 270,
  270, 90,
  90, 90
};

const t_digit digit_5 = {
  270, 0,
  90, 0,
  0, 0,
  180, 180,
  270, 180,
  90, 180
};

const t_digit digit_6 = {
  270, 0,
  270, 90,
  90, 0,
  180, 180,
  270, 180,
  90, 180
};

const t_digit digit_7 = {
  0, 0,
  225, 225,
  225, 225,
  270, 180,
  270, 90,
  90, 90
};

const t_digit digit_8 = {
  270, 0,
  90, 0,
  90, 0,
  270, 180,
  90, 180,
  90, 180
};

const t_digit digit_9 = {
  270, 0,
  0, 90,
  0, 0,
  270, 180,
  270, 90,
  90, 180
};

const t_digit digit_null = {
  270, 270,
  270, 270,
  270, 270,
  270, 270,
  270, 270,
  270, 270
};

const t_digit digit_I = {
  270, 90,
  270, 90,
  270, 90,
  270, 90,
  270, 90,
  270, 90
};

const t_digit digit_fun = {
  225, 45,
  225, 45,
  225, 45,
  225, 45,
  225, 45,
  225, 45,
};

const t_full_clock d_stop = {digit_null, digit_null, digit_null, digit_null};

const t_full_clock d_fun = {digit_fun, digit_fun, digit_fun, digit_fun};

const t_full_clock d_IIII = {digit_I, digit_I, digit_I, digit_I};

/**
 * Fluid-wave intermediate poses (t_half_digitl — lite, angles only).
 * All three clocks in each half-digit share the same angles except
 * POSE_WAVE_BREATH, which differentiates by row.
 *
 * Angles follow the firmware convention: 0° = 12 o'clock, CW positive.
 */

// Hands 150° apart, symmetric about the 6-o'clock axis  — open arc shape
const t_half_digitl POSE_WAVE_OPEN      = { 105, 255,  105, 255,  105, 255 };

// < bracket shape  (hands at 120° / 240°)
const t_half_digitl POSE_WAVE_BKT_LEFT  = { 120, 240,  120, 240,  120, 240 };

// > bracket shape  (hands at 60° / 300°)
const t_half_digitl POSE_WAVE_BKT_RIGHT = {  60, 300,   60, 300,   60, 300 };

// Horizontal — shape  (hands at 90° / 270°)
const t_half_digitl POSE_WAVE_FLAT      = {  90, 270,   90, 270,   90, 270 };

// Vertical | shape  (hands at 0° / 180°)
const t_half_digitl POSE_WAVE_VERTICAL  = {   0, 180,    0, 180,    0, 180 };

// ^ up-arrow shape  (hands at 45° / 315°)
const t_half_digitl POSE_WAVE_UP_ARROW  = {  45, 315,   45, 315,   45, 315 };

// v down-arrow shape  (hands at 135° / 225°)
const t_half_digitl POSE_WAVE_DOWN_ARROW = { 135, 225,  135, 225,  135, 225 };

// Phase-3 "breathing" pose — row 0 (top) = ^, row 1 (mid) = —, row 2 (bot) = v
const t_half_digitl POSE_WAVE_BREATH    = {  45, 315,   90, 270,  135, 225 };

#endif