#pragma once
#include "lcd_api.hpp"
#include <cstdint>

/**
 * @file gpio_lcd.hpp
 * @brief HD44780 character LCD driven directly via Pico GPIO pins.
 *
 * Supports both 4-bit and 8-bit bus modes.  For 4-bit mode (most common),
 * only supply d4..d7 and leave d0..d3 as UINT_MAX (the default sentinel).
 *
 * Pin numbers are bare GPIO numbers (0-29), matching the Pico SDK convention.
 * Pass GPIO_NONE for unused optional pins (rw_pin, backlight_pin).
 */

static constexpr uint GPIO_NONE = 0xFFFF;

class GpioLcd : public LcdApi {
public:
    /**
     * @param rs_pin        GPIO for Register Select
     * @param enable_pin    GPIO for Enable
     * @param d4..d7        GPIO for data bits 4-7  (always required)
     * @param d0..d3        GPIO for data bits 0-3  (8-bit mode only, else GPIO_NONE)
     * @param rw_pin        GPIO for R/W             (optional, else GPIO_NONE)
     * @param backlight_pin GPIO for backlight        (optional, else GPIO_NONE)
     * @param num_lines     Number of LCD rows
     * @param num_columns   Number of LCD columns
     */
    GpioLcd(uint rs_pin, uint enable_pin,
            uint d4_pin, uint d5_pin, uint d6_pin, uint d7_pin,
            uint d0_pin      = GPIO_NONE,
            uint d1_pin      = GPIO_NONE,
            uint d2_pin      = GPIO_NONE,
            uint d3_pin      = GPIO_NONE,
            uint rw_pin      = GPIO_NONE,
            uint backlight_pin = GPIO_NONE,
            int  num_lines   = 2,
            int  num_columns = 16);

protected:
    void hal_write_command(uint8_t cmd) override;
    void hal_write_data(uint8_t data)   override;
    void hal_backlight_on()  override;
    void hal_backlight_off() override;

private:
    uint rs_pin, enable_pin;
    uint d4_pin, d5_pin, d6_pin, d7_pin;
    uint d0_pin, d1_pin, d2_pin, d3_pin;
    uint rw_pin, backlight_pin;
    bool mode_4bit;

    void pin_out_init(uint pin);
    void pin_put(uint pin, bool val);
    void hal_pulse_enable();
    void hal_write_init_nibble(uint8_t nibble);
    void hal_write_8bits(uint8_t value);
    void hal_write_4bits(uint8_t nibble);
};
