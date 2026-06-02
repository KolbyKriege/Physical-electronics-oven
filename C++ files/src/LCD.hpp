#pragma once
#include <cstdint>

/**
 * @file LCD.hpp
 * @brief High-level display module for the reflow oven controller.
 *
 * Manages a 20×4 character LCD. All state lives in main.cpp; this module
 * only handles formatting and rendering.
 *
 * Display layout (20 cols × 4 rows):
 *   Row 0: [THERM] CCCC/SSSS  <cursor>
 *   Row 1: [CLOCK] MM:SS/MM:SS <cursor>
 *   Row 2: >P:PP  >I:II  >D:DD
 *   Row 3: START<  (or mode info / errors)
 *
 * State codes:
 *   0  EDIT_TEMP    5  START
 *   1  EDIT_TIME   10  RUNNING
 *   2  EDIT_P      20  ERROR_TEMP
 *   3  EDIT_I      21  ERROR_TIME
 *   4  EDIT_D      22  ERROR_PID
 */
namespace LCD {

void setup();

/**
 * @param state     Current state machine state (see above).
 * @param set_temp  Target temperature (°C).
 * @param curr_temp Current measured temperature (°C).
 * @param set_time  Target duration (seconds).
 * @param curr_time Elapsed / remaining time (seconds).
 * @param P         Proportional gain.
 * @param I         Integral gain.
 * @param D         Derivative gain.
 */
void update(int state,
            int   set_temp,  float curr_temp,
            int   set_time,  float curr_time,
            float P, float I, float D);

} // namespace LCD
