#pragma once
#include <cstdint>

/**
 * @file ssr.hpp
 * @brief Solid State Relay (SSR) driver using hardware PWM.
 *
 * Controls the SSR on GPIO 0 via the Pico's hardware PWM peripheral.
 * Duty cycle range: 0.0 (off) to 100.0 (full on).
 */
namespace SSR {

/**
 * Initialise PWM on GPIO 0.
 * @param frequency  Desired PWM frequency in Hz (default 60 Hz).
 */
void setup(uint32_t frequency = 60);

/**
 * Set SSR power level.
 * @param duty_cycle  Percentage [0.0, 100.0].
 */
void set_power(float duty_cycle);

/** Immediately cut power to the SSR (duty cycle → 0). */
void shutdown();

} // namespace SSR
