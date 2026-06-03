#pragma once
#include "lcd_api.hpp"
#include <cstdint>
#include <pico/types.h>

/**
 * @file gpio_lcd.hpp
 * @brief HD44780 character LCD driven directly via Pico GPIO pins.
 *
 * Supports both 4-bit mode (d4-d7 only) and 8-bit mode (d0-d7).
 * Pass GPIO_NONE for unused pins (d0-d3 in 4-bit mode, rw_pin, backlight_pin).
 *
 * NOTE on constructor argument order: d4-d7 come BEFORE d0-d3 so that
 * 4-bit-mode callers can pass just 4 pins positionally without naming them.
 */

static constexpr uint GPIO_NONE = 0xFFFF;

class GpioLcd : public LcdApi {
public:
    GpioLcd(uint rs_pin, uint enable_pin,
            uint d4_pin, uint d5_pin, uint d6_pin, uint d7_pin,
            uint d0_pin        = GPIO_NONE,
            uint d1_pin        = GPIO_NONE,
            uint d2_pin        = GPIO_NONE,
            uint d3_pin        = GPIO_NONE,
            uint rw_pin        = GPIO_NONE,
            uint backlight_pin = GPIO_NONE,
            int  num_lines     = 2,
            int  num_columns   = 16);

protected:
    void hal_write_command(uint8_t cmd) override;
    void hal_write_data(uint8_t data)   override;
    void hal_backlight_on()             override;
    void hal_backlight_off()            override;

private:
    uint rs_pin, enable_pin;
    uint d4_pin, d5_pin, d6_pin, d7_pin;
    uint d0_pin, d1_pin, d2_pin, d3_pin;
    uint rw_pin, backlight_pin;
    bool mode_4bit;

    void pin_out_init(uint pin);
    void pin_put(uint pin, bool val);

    void hal_pulse_enable();
    void hal_write_4bits(uint8_t nibble);
    void hal_write_init_nibble(uint8_t byte_val);
    void hal_write_8bits_raw(uint8_t value);
    void hal_write_command_raw(uint8_t cmd);
};
