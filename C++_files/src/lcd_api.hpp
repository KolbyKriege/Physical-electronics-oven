#pragma once
#include <cstdint>
#include <cstring>

/**
 * @file lcd_api.hpp
 * @brief HD44780 compatible character LCD base class (hardware-agnostic).
 *
 * Derived classes must implement hal_write_command() and hal_write_data().
 * They perform the full HD44780 hardware init in their own constructor.
 */
class LcdApi {
public:
    static constexpr uint8_t LCD_CLR             = 0x01;
    static constexpr uint8_t LCD_HOME            = 0x02;

    static constexpr uint8_t LCD_ENTRY_MODE      = 0x04;
    static constexpr uint8_t LCD_ENTRY_INC       = 0x02;
    static constexpr uint8_t LCD_ENTRY_SHIFT     = 0x01;

    static constexpr uint8_t LCD_ON_CTRL         = 0x08;
    static constexpr uint8_t LCD_ON_DISPLAY      = 0x04;
    static constexpr uint8_t LCD_ON_CURSOR       = 0x02;
    static constexpr uint8_t LCD_ON_BLINK        = 0x01;

    static constexpr uint8_t LCD_MOVE            = 0x10;
    static constexpr uint8_t LCD_MOVE_DISP       = 0x08;
    static constexpr uint8_t LCD_MOVE_RIGHT      = 0x04;

    static constexpr uint8_t LCD_FUNCTION        = 0x20;
    static constexpr uint8_t LCD_FUNCTION_8BIT   = 0x10;
    static constexpr uint8_t LCD_FUNCTION_2LINES = 0x08;
    static constexpr uint8_t LCD_FUNCTION_10DOTS = 0x04;
    static constexpr uint8_t LCD_FUNCTION_RESET  = 0x30;

    static constexpr uint8_t LCD_CGRAM           = 0x40;
    static constexpr uint8_t LCD_DDRAM           = 0x80;

    LcdApi(int num_lines, int num_columns);
    virtual ~LcdApi() = default;

    void clear();
    void show_cursor();
    void hide_cursor();
    void blink_cursor_on();
    void blink_cursor_off();
    void display_on();
    void display_off();
    void backlight_on();
    void backlight_off();

    /**
     * Move hardware cursor to column x, row y (both zero-based).
     * Sends a DDRAM set command — use sparingly, only when repositioning.
     */
    void move_to(int x, int y);

    void putchar(char c);
    void putstr(const char* str);

    /**
     * Write exactly num_columns characters to the given row.
     * Always starts with move_to(0, row), then writes data bytes directly
     * (no mid-row DDRAM commands). Pads short strings with spaces.
     * This is the preferred way to update a full row.
     */
    void write_row(int row, const char* str);

    void custom_char(uint8_t location, const uint8_t charmap[8]);

protected:
    int  num_lines;
    int  num_columns;
    int  cursor_x;
    int  cursor_y;
    bool implied_newline;
    bool backlight_state;

    virtual void hal_write_command(uint8_t cmd) = 0;
    virtual void hal_write_data(uint8_t data)   = 0;
    virtual void hal_backlight_on()  {}
    virtual void hal_backlight_off() {}
    virtual void hal_sleep_us(uint32_t usecs);
};
