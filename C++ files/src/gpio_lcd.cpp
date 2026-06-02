#include "gpio_lcd.hpp"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ── helpers ──────────────────────────────────────────────────────────────────

void GpioLcd::pin_out_init(uint pin) {
    if (pin == GPIO_NONE) return;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void GpioLcd::pin_put(uint pin, bool val) {
    if (pin == GPIO_NONE) return;
    gpio_put(pin, val ? 1 : 0);
}

// ── constructor ───────────────────────────────────────────────────────────────

GpioLcd::GpioLcd(uint rs_pin, uint enable_pin,
                 uint d4_pin, uint d5_pin, uint d6_pin, uint d7_pin,
                 uint d0_pin, uint d1_pin, uint d2_pin, uint d3_pin,
                 uint rw_pin, uint backlight_pin,
                 int num_lines, int num_columns)
    : LcdApi(num_lines, num_columns),
      rs_pin(rs_pin), enable_pin(enable_pin),
      d4_pin(d4_pin), d5_pin(d5_pin), d6_pin(d6_pin), d7_pin(d7_pin),
      d0_pin(d0_pin), d1_pin(d1_pin), d2_pin(d2_pin), d3_pin(d3_pin),
      rw_pin(rw_pin), backlight_pin(backlight_pin)
{
    // Determine bus width
    mode_4bit = (d0_pin == GPIO_NONE || d1_pin == GPIO_NONE ||
                 d2_pin == GPIO_NONE || d3_pin == GPIO_NONE);

    // Initialise output pins
    pin_out_init(rs_pin);
    pin_out_init(enable_pin);
    pin_out_init(rw_pin);
    pin_out_init(backlight_pin);
    pin_out_init(d4_pin);
    pin_out_init(d5_pin);
    pin_out_init(d6_pin);
    pin_out_init(d7_pin);
    if (!mode_4bit) {
        pin_out_init(d0_pin);
        pin_out_init(d1_pin);
        pin_out_init(d2_pin);
        pin_out_init(d3_pin);
    }

    // LCD power-on reset sequence
    sleep_ms(20);
    hal_write_init_nibble(LCD_FUNCTION_RESET);
    sleep_ms(5);
    hal_write_init_nibble(LCD_FUNCTION_RESET);
    sleep_ms(1);
    hal_write_init_nibble(LCD_FUNCTION_RESET);
    sleep_ms(1);

    uint8_t cmd = LCD_FUNCTION;
    if (!mode_4bit) cmd |= LCD_FUNCTION_8BIT;
    hal_write_init_nibble(cmd);
    sleep_ms(1);

    // Base class init (calls hal_write_command, etc.)
    LcdApi::LcdApi(num_lines, num_columns);   // re-invoke would double-init; call parent cmds instead
    if (num_lines > 1) cmd |= LCD_FUNCTION_2LINES;
    hal_write_command(cmd);
}

// ── private helpers ───────────────────────────────────────────────────────────

void GpioLcd::hal_pulse_enable() {
    pin_put(enable_pin, false);
    sleep_us(1);
    pin_put(enable_pin, true);
    sleep_us(1);    // pulse > 450 ns
    pin_put(enable_pin, false);
    sleep_us(100);  // commands need > 37 µs to settle
}

void GpioLcd::hal_write_init_nibble(uint8_t nibble) {
    hal_write_4bits(nibble >> 4);
}

void GpioLcd::hal_write_4bits(uint8_t nibble) {
    pin_put(d7_pin, nibble & 0x08);
    pin_put(d6_pin, nibble & 0x04);
    pin_put(d5_pin, nibble & 0x02);
    pin_put(d4_pin, nibble & 0x01);
    hal_pulse_enable();
}

void GpioLcd::hal_write_8bits(uint8_t value) {
    pin_put(rw_pin, false);
    if (mode_4bit) {
        hal_write_4bits(value >> 4);
        hal_write_4bits(value);
    } else {
        pin_put(d3_pin, value & 0x08);
        pin_put(d2_pin, value & 0x04);
        pin_put(d1_pin, value & 0x02);
        pin_put(d0_pin, value & 0x01);
        hal_write_4bits(value >> 4);
    }
}

// ── LcdApi overrides ──────────────────────────────────────────────────────────

void GpioLcd::hal_write_command(uint8_t cmd) {
    pin_put(rs_pin, false);
    hal_write_8bits(cmd);
    if (cmd <= 3) {
        sleep_ms(5);  // clear / home worst-case delay
    }
}

void GpioLcd::hal_write_data(uint8_t data) {
    pin_put(rs_pin, true);
    hal_write_8bits(data);
}

void GpioLcd::hal_backlight_on() {
    pin_put(backlight_pin, true);
}

void GpioLcd::hal_backlight_off() {
    pin_put(backlight_pin, false);
}
