#include "gpio_lcd.hpp"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ── helpers ───────────────────────────────────────────────────────────────────

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
    // Determine bus width: 4-bit if any of d0-d3 are absent
    mode_4bit = (d0_pin == GPIO_NONE || d1_pin == GPIO_NONE ||
                 d2_pin == GPIO_NONE || d3_pin == GPIO_NONE);

    // Initialise all output pins low
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

    // ── HD44780 power-on reset sequence (datasheet §4, "Initializing by Instruction") ──
    sleep_ms(50);   // Wait >40ms after Vcc reaches 4.5V

    // Send reset nibble/byte three times as required by the datasheet
    if (mode_4bit) {
        hal_write_init_nibble(LCD_FUNCTION_RESET);
        sleep_ms(5);
        hal_write_init_nibble(LCD_FUNCTION_RESET);
        sleep_ms(1);
        hal_write_init_nibble(LCD_FUNCTION_RESET);
        sleep_ms(1);

        // Now switch to 4-bit mode
        hal_write_init_nibble(LCD_FUNCTION);
        sleep_ms(1);
    } else {
        // 8-bit mode: send full byte three times
        hal_write_command_raw(LCD_FUNCTION_RESET);
        sleep_ms(5);
        hal_write_command_raw(LCD_FUNCTION_RESET);
        sleep_ms(1);
        hal_write_command_raw(LCD_FUNCTION_RESET);
        sleep_ms(1);
    }

    // Function set: bus width + number of lines
    uint8_t func_cmd = LCD_FUNCTION;
    if (!mode_4bit)    func_cmd |= LCD_FUNCTION_8BIT;
    if (num_lines > 1) func_cmd |= LCD_FUNCTION_2LINES;
    hal_write_command(func_cmd);
    sleep_ms(1);

    // Display off
    hal_write_command(LCD_ON_CTRL);
    sleep_ms(1);

    // Display clear
    hal_write_command(LCD_CLR);
    sleep_ms(5);

    // Entry mode: increment, no shift
    hal_write_command(LCD_ENTRY_MODE | LCD_ENTRY_INC);
    sleep_ms(1);

    // Display on, cursor off, blink off
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY);
    sleep_ms(1);
}

// ── private: pulse Enable once ────────────────────────────────────────────────

void GpioLcd::hal_pulse_enable() {
    pin_put(enable_pin, false);
    sleep_us(1);
    pin_put(enable_pin, true);
    sleep_us(1);      // E pulse width must be > 450 ns
    pin_put(enable_pin, false);
    sleep_us(50);     // HD44780 needs >37µs to complete most commands
}

// ── private: write 4 bits (D4-D7) and pulse Enable ───────────────────────────
//
// Bit mapping:   nibble bit3 → D7, bit2 → D6, bit1 → D5, bit0 → D4
//
void GpioLcd::hal_write_4bits(uint8_t nibble) {
    pin_put(d4_pin, nibble & 0x01);
    pin_put(d5_pin, nibble & 0x02);
    pin_put(d6_pin, nibble & 0x04);
    pin_put(d7_pin, nibble & 0x08);
    hal_pulse_enable();
}

// ── private: write init nibble (used only during power-on reset) ──────────────

void GpioLcd::hal_write_init_nibble(uint8_t byte_val) {
    // Send only the upper nibble, used before 4-bit mode is established
    hal_write_4bits(byte_val >> 4);
}

// ── private: write all 8 bits and pulse Enable once (8-bit bus) ───────────────
//
// In 8-bit mode ALL 8 data pins are set, then Enable is pulsed ONCE.
// D0=bit0, D1=bit1, D2=bit2, D3=bit3, D4=bit4, D5=bit5, D6=bit6, D7=bit7
//
void GpioLcd::hal_write_8bits_raw(uint8_t value) {
    pin_put(d0_pin, value & 0x01);
    pin_put(d1_pin, value & 0x02);
    pin_put(d2_pin, value & 0x04);
    pin_put(d3_pin, value & 0x08);
    pin_put(d4_pin, value & 0x10);
    pin_put(d5_pin, value & 0x20);
    pin_put(d6_pin, value & 0x40);
    pin_put(d7_pin, value & 0x80);
    hal_pulse_enable();
}

// ── private: raw command send (no RS management) ──────────────────────────────

void GpioLcd::hal_write_command_raw(uint8_t cmd) {
    pin_put(rs_pin, false);
    if (rw_pin != GPIO_NONE) pin_put(rw_pin, false);
    hal_write_8bits_raw(cmd);
}

// ── LcdApi overrides ──────────────────────────────────────────────────────────

void GpioLcd::hal_write_command(uint8_t cmd) {
    pin_put(rs_pin, false);
    if (rw_pin != GPIO_NONE) pin_put(rw_pin, false);

    if (mode_4bit) {
        hal_write_4bits(cmd >> 4);   // upper nibble first
        hal_write_4bits(cmd & 0x0F); // then lower nibble
    } else {
        hal_write_8bits_raw(cmd);
    }

    // Clear (0x01) and Home (0x02) need up to 1.52ms; give them 5ms to be safe
    if (cmd <= 3) {
        sleep_ms(5);
    } else {
        sleep_us(50);
    }
}

void GpioLcd::hal_write_data(uint8_t data) {
    pin_put(rs_pin, true);
    if (rw_pin != GPIO_NONE) pin_put(rw_pin, false);

    if (mode_4bit) {
        hal_write_4bits(data >> 4);
        hal_write_4bits(data & 0x0F);
    } else {
        hal_write_8bits_raw(data);
    }

    sleep_us(50);
}

void GpioLcd::hal_backlight_on() {
    pin_put(backlight_pin, true);
}

void GpioLcd::hal_backlight_off() {
    pin_put(backlight_pin, false);
}
