#pragma once
#include "hardware/gpio.h"
#include <cstdint>
#include <atomic>

/**
 * @file rotary.hpp
 * @brief Interrupt-driven rotary encoder driver for Raspberry Pi Pico.
 *
 * Uses a full-step state-machine identical to the MicroPython original.
 * The encoder value is updated from GPIO IRQ handlers (core-safe via
 * std::atomic).  Call value() to read and reset_delta() to consume ticks.
 *
 * Only one RotaryEncoder instance is supported globally (ISR limitation).
 */
class RotaryEncoder {
public:
    /**
     * @param clk_pin   GPIO for CLK signal (with internal pull-up).
     * @param dt_pin    GPIO for DT  signal (with internal pull-up).
     */
    RotaryEncoder(uint clk_pin, uint dt_pin);

    /** Current absolute position (unbounded, can be negative). */
    int value() const;

    /** Reset absolute position to 0. */
    void reset();

    /**
     * Return how many detents have moved since the last call and reset the
     * internal delta counter to 0.  Positive = clockwise.
     */
    int consume_delta();

private:
    uint clk_pin, dt_pin;
    std::atomic<int> pos;
    std::atomic<int> delta;

    // State-machine state (matches MicroPython rotary.py table)
    volatile uint8_t enc_state;

    static void gpio_irq_handler(uint gpio, uint32_t events);
    void        process();

    // Singleton pointer used by the static ISR
    static RotaryEncoder* instance_;
};
