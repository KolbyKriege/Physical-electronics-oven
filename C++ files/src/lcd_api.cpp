#include "lcd_api.hpp"
#include "pico/stdlib.h"
#include <algorithm>

LcdApi::LcdApi(int num_lines, int num_columns)
    : num_lines(std::min(num_lines, 4)),
      num_columns(std::min(num_columns, 40)),
      cursor_x(0), cursor_y(0),
      implied_newline(false), backlight_state(true)
{
    display_off();
    backlight_on();
    clear();
    hal_write_command(LCD_ENTRY_MODE | LCD_ENTRY_INC);
    hide_cursor();
    display_on();
}

void LcdApi::clear() {
    hal_write_command(LCD_CLR);
    hal_write_command(LCD_HOME);
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
    uint8_t addr = x & 0x3F;
    if (y & 1) addr += 0x40;          // rows 1 & 3
    if (y & 2) addr += num_columns;   // rows 2 & 3
    hal_write_command(LCD_DDRAM | addr);
}

void LcdApi::putchar(char c) {
    if (c == '\n') {
        if (implied_newline) {
            implied_newline = false;
        } else {
            cursor_x = num_columns; // force wrap
        }
    } else {
        hal_write_data(static_cast<uint8_t>(c));
        cursor_x++;
    }

    if (cursor_x >= num_columns) {
        cursor_x = 0;
        cursor_y++;
        implied_newline = (c != '\n');
    }
    if (cursor_y >= num_lines) {
        cursor_y = 0;
    }
    move_to(cursor_x, cursor_y);
}

void LcdApi::putstr(const char* str) {
    while (*str) {
        putchar(*str++);
    }
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
