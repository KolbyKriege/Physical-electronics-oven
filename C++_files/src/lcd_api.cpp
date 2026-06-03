#include "lcd_api.hpp"
#include "pico/stdlib.h"
#include <algorithm>
#include <cstring>

// ── DDRAM row-start addresses ─────────────────────────────────────────────────
//
// 20x4 HD44780 modules come in two physical wiring variants:
//
//   Layout A (standard):  top=0x00, 0x40, 0x14, 0x54
//   Layout B (alternate): top=0x40, 0x00, 0x54, 0x14
//
// Symptom of wrong layout: the top physical row is always blank.
// This firmware uses Layout B because the attached module's top row
// is wired to the second controller (0x40).
//
// To switch back to Layout A, change the order to: 0x00, 0x40, 0x14, 0x54
//
static constexpr uint8_t ROW_OFFSETS[4] = { 0x00, 0x40, 0x14, 0x54 };

LcdApi::LcdApi(int num_lines, int num_columns)
    : num_lines(std::min(num_lines, 4)),
      num_columns(std::min(num_columns, 40)),
      cursor_x(0), cursor_y(0),
      implied_newline(false), backlight_state(true)
{
}

void LcdApi::clear() {
    hal_write_command(LCD_CLR);
    // hal_write_command already waits 5 ms for cmd <= 3, but many HD44780
    // clones need longer after a clear before they will accept a DDRAM
    // address command. Add an extra 5 ms to guarantee the controller is
    // fully idle before the first write_row / move_to call.
    sleep_ms(5);
    cursor_x = 0;
    cursor_y = 0;
}

void LcdApi::show_cursor() {
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY | LCD_ON_CURSOR);
}

void LcdApi::hide_cursor() {
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY);
}

void LcdApi::blink_cursor_on() {
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY | LCD_ON_CURSOR | LCD_ON_BLINK);
}

void LcdApi::blink_cursor_off() {
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY | LCD_ON_CURSOR);
}

void LcdApi::display_on() {
    hal_write_command(LCD_ON_CTRL | LCD_ON_DISPLAY);
}

void LcdApi::display_off() {
    hal_write_command(LCD_ON_CTRL);
}

void LcdApi::backlight_on() {
    backlight_state = true;
    hal_backlight_on();
}

void LcdApi::backlight_off() {
    backlight_state = false;
    hal_backlight_off();
}

void LcdApi::move_to(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    uint8_t addr = ROW_OFFSETS[y & 3] + static_cast<uint8_t>(x);
    hal_write_command(LCD_DDRAM | addr);
}

void LcdApi::putchar(char c) {
    if (c == '\n') {
        if (implied_newline) {
            implied_newline = false;
        } else {
            cursor_x = num_columns;
        }
    } else {
        hal_write_data(static_cast<uint8_t>(c));
        cursor_x++;
    }

    if (cursor_x >= num_columns) {
        cursor_x = 0;
        cursor_y++;
        implied_newline = (c != '\n');
        if (cursor_y >= num_lines) cursor_y = 0;
        move_to(cursor_x, cursor_y);
    }
}

void LcdApi::putstr(const char* str) {
    while (*str) putchar(*str++);
}

void LcdApi::write_row(int row, const char* str) {
    move_to(0, row);
    int len = static_cast<int>(strlen(str));
    for (int i = 0; i < num_columns; i++) {
        hal_write_data(i < len ? static_cast<uint8_t>(str[i]) : ' ');
    }
    cursor_x = 0;
    cursor_y = (row + 1) % num_lines;
}

void LcdApi::custom_char(uint8_t location, const uint8_t charmap[8]) {
    location &= 0x07;
    hal_write_command(LCD_CGRAM | (location << 3));
    hal_sleep_us(40);
    for (int i = 0; i < 8; i++) {
        hal_write_data(charmap[i]);
        hal_sleep_us(40);
    }
    move_to(cursor_x, cursor_y);
}

void LcdApi::hal_sleep_us(uint32_t usecs) {
    sleep_us(usecs);
}
