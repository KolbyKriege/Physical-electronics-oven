#include "LCD.hpp"
#include "gpio_lcd.hpp"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

static GpioLcd* lcd = nullptr;

static const uint8_t TEMP_CHAR[8] = {0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x11, 0x0E};
static const uint8_t TIME_CHAR[8] = {0x00, 0x00, 0x0E, 0x13, 0x15, 0x11, 0x0E, 0x00};

// CGRAM slot indices for custom chars
static constexpr uint8_t CGRAM_TEMP = 0;
static constexpr uint8_t CGRAM_TIME = 1;

static void fmt_time(char* buf, int seconds) {
    seconds = std::max(0, seconds);
    snprintf(buf, 6, "%02d:%02d", seconds / 60, seconds % 60);
}

// Write one row that starts with a CGRAM custom character (index 0-7),
// followed by a snprintf-formatted string for the rest.
// We can't put CGRAM indices through snprintf because \x00 looks like
// a null terminator and kills strlen. Instead we write the custom char
// byte directly, then write the rest of the string, padding to 20 cols.
static void write_row_custom(int row, uint8_t cgram_idx,
                              const char* rest, int rest_len)
{
    lcd->move_to(0, row);
    // Write the custom char directly as a data byte
    lcd->putchar(static_cast<char>(cgram_idx));
    // Write the rest, padding to fill all 20 columns (1 used by custom char)
    for (int i = 0; i < 19; i++) {
        lcd->putchar(i < rest_len ? rest[i] : ' ');
    }
}

namespace LCD {

void setup() {
    static GpioLcd lcd_instance(
        /*rs_pin*/      12,
        /*enable_pin*/  10,
        /*d4_pin*/       6,
        /*d5_pin*/       7,
        /*d6_pin*/       8,
        /*d7_pin*/       9,
        /*d0_pin*/       2,
        /*d1_pin*/       3,
        /*d2_pin*/       4,
        /*d3_pin*/       5,
        /*rw_pin*/       GPIO_NONE,
        /*backlight*/    GPIO_NONE,
        /*lines*/        4,
        /*columns*/     20
    );
    lcd = &lcd_instance;

    lcd->custom_char(CGRAM_TEMP, TEMP_CHAR);
    lcd->custom_char(CGRAM_TIME, TIME_CHAR);
}

void update(int state,
            int   set_temp,  float curr_temp,
            int   set_time,  float curr_time,
            float P, float I, float D)
{
    if (!lcd) return;

    char ct[6], st[6];
    fmt_time(ct, (int)curr_time);
    fmt_time(st, set_time);

    // ── Error screens ─────────────────────────────────────────────────────────
    if (state == 20) {
        lcd->clear();
        lcd->write_row(0, "ERROR:Temp too low");
        lcd->write_row(1, "Target below ambient");
        lcd->write_row(2, "");
        lcd->write_row(3, "Fix set temp & retry");
        return;
    }
    if (state == 21) {
        lcd->clear();
        lcd->write_row(0, "ERROR: Invalid time");
        lcd->write_row(1, "Set time must be > 0");
        lcd->write_row(2, "");
        lcd->write_row(3, "Fix set time & retry");
        return;
    }
    if (state == 22) {
        lcd->clear();
        lcd->write_row(0, "ERROR: Bad PID gains");
        lcd->write_row(1, "P, I, D must be >= 0");
        lcd->write_row(2, "");
        lcd->write_row(3, "Fix PID & retry");
        return;
    }

    // ── Running screen ────────────────────────────────────────────────────────
    if (state == 10) {
        // Row 0: [THERM]:%04.0f/%04d   e.g. "T:0023/0500"
        char tmp0[20];
        int  len0 = snprintf(tmp0, sizeof(tmp0), ":%04.0f/%04d", curr_temp, set_temp);

        // Row 1: [CLOCK]:%s/%s          e.g. "C:01:00/01:00"
        char tmp1[20];
        int  len1 = snprintf(tmp1, sizeof(tmp1), ":%s/%s", ct, st);

        char row2[21];
        snprintf(row2, sizeof(row2), "P:%.1f I:%.1f D:%.1f", P, I, D);

        lcd->clear();
        write_row_custom(0, CGRAM_TEMP, tmp0, len0);
        write_row_custom(1, CGRAM_TIME, tmp1, len1);
        lcd->write_row(2, row2);
        lcd->write_row(3, "Press Encoder 2 STOP");
        return;
    }

    // ── Normal navigation / edit screens ──────────────────────────────────────
    char t_cur  = (state == 0) ? '<' : ' ';
    char tm_cur = (state == 1) ? '<' : ' ';
    char p_cur  = (state == 2) ? '>' : ' ';
    char i_cur  = (state == 3) ? '>' : ' ';
    char d_cur  = (state == 4) ? '>' : ' ';
    char s_cur  = (state == 5) ? '<' : ' ';

    // Row 0: [THERM]:%04.0f/%04d<    e.g. "T:0023/0500<"
    char tmp0[20];
    int  len0 = snprintf(tmp0, sizeof(tmp0), ":%04.0f/%04d%c", curr_temp, set_temp, t_cur);

    // Row 1: [CLOCK]:%s/%s<           e.g. "C:01:00/01:00<"
    char tmp1[20];
    int  len1 = snprintf(tmp1, sizeof(tmp1), ":%s/%s%c", ct, st, tm_cur);

    char row2[21], row3[21];
    snprintf(row2, sizeof(row2), "%cP:%.1f %cI:%.1f %cD:%.1f",
             p_cur, P, i_cur, I, d_cur, D);
    snprintf(row3, sizeof(row3), "START%c", s_cur);

    lcd->clear();
    write_row_custom(0, CGRAM_TEMP, tmp0, len0);
    write_row_custom(1, CGRAM_TIME, tmp1, len1);
    lcd->write_row(2, row2);
    lcd->write_row(3, row3);
}

} // namespace LCD
