#include "LCD.hpp"
#include "gpio_lcd.hpp"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// ── Pin assignments (match MicroPython original) ──────────────────────────────
// GPIO 12 = RS, GPIO 10 = Enable
// GPIO 2-9 = D0-D7 (8-bit mode)
static GpioLcd lcd(
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

// Custom character bitmaps
static const uint8_t TEMP_CHAR[8] = {0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x11, 0x0E};
static const uint8_t TIME_CHAR[8] = {0x00, 0x00, 0x0E, 0x13, 0x15, 0x11, 0x0E, 0x00};

// Custom char indices
static constexpr char CHR_TEMP = '\x00';
static constexpr char CHR_TIME = '\x01';

// ── helpers ───────────────────────────────────────────────────────────────────

static void fmt_time(char* buf, int seconds) {
    seconds = std::max(0, seconds);
    int m = seconds / 60;
    int s = seconds % 60;
    snprintf(buf, 6, "%02d:%02d", m, s);
}

// Write exactly `width` characters from `src` to the lcd, padding with spaces.
static void lcd_field(const char* src, int width) {
    int len = (int)strlen(src);
    for (int i = 0; i < width; i++) {
        lcd.putchar(i < len ? src[i] : ' ');
    }
}

// ── public API ────────────────────────────────────────────────────────────────

namespace LCD {

void setup() {
    lcd.custom_char(0, TEMP_CHAR);
    lcd.custom_char(1, TIME_CHAR);
}

void update(int state,
            int   set_temp,  float curr_temp,
            int   set_time,  float curr_time,
            float P, float I, float D)
{
    char ct[6], st[6];
    fmt_time(ct, (int)curr_time);
    fmt_time(st, set_time);

    // ── Error screens ────────────────────────────────────────────────────────
    if (state == 20) {
        lcd.clear();
        lcd.putstr("ERROR:Temp too low  "
                   "Target below ambient"
                   "                    "
                   "Fix set temp & retry");
        return;
    }
    if (state == 21) {
        lcd.clear();
        lcd.putstr("ERROR: Invalid time "
                   "Set time must be > 0"
                   "                    "
                   "Fix set time & retry");
        return;
    }
    if (state == 22) {
        lcd.clear();
        lcd.putstr("ERROR: Bad PID gains"
                   "P, I, D must be >= 0"
                   "                    "
                   "Fix PID & retry     ");
        return;
    }

    // ── Running screen ───────────────────────────────────────────────────────
    if (state == 10) {
        char row0[21], row1[21], row2[21];

        // Row 0: THERM CCCC/SSSS + padding to 20
        snprintf(row0, sizeof(row0), "%c:%04.0f/%04d", CHR_TEMP, curr_temp, set_temp);
        // Row 1: CLOCK MM:SS/MM:SS
        snprintf(row1, sizeof(row1), "%c:%s/%s", CHR_TIME, ct, st);
        // Row 2: P / I / D values
        snprintf(row2, sizeof(row2), "P:%.1f I:%.1f D:%.1f", P, I, D);

        lcd.clear();
        lcd_field(row0, 20);
        lcd_field(row1, 20);
        lcd_field(row2, 20);
        lcd_field("Press Encoder 2 STOP", 20);
        return;
    }

    // ── Normal navigation / edit screens ─────────────────────────────────────
    char t_cur  = (state == 0) ? '<' : ' ';
    char tm_cur = (state == 1) ? '<' : ' ';
    char p_cur  = (state == 2) ? '>' : ' ';
    char i_cur  = (state == 3) ? '>' : ' ';
    char d_cur  = (state == 4) ? '>' : ' ';
    char s_cur  = (state == 5) ? '<' : ' ';

    char row0[21], row1[21], row2[21], row3[21];

    // Row 0: THERM CCCC/SSSS <cursor>
    snprintf(row0, sizeof(row0), "%c:%04.0f/%04d%c",
             CHR_TEMP, curr_temp, set_temp, t_cur);
    // Row 1: CLOCK MM:SS/MM:SS <cursor>
    snprintf(row1, sizeof(row1), "%c:%s/%s%c",
             CHR_TIME, ct, st, tm_cur);
    // Row 2: >P:x.x >I:x.x >D:x.x
    snprintf(row2, sizeof(row2), "%cP:%.1f %cI:%.1f %cD:%.1f",
             p_cur, P, i_cur, I, d_cur, D);
    // Row 3: START<
    snprintf(row3, sizeof(row3), "START%c", s_cur);

    lcd.clear();
    lcd_field(row0, 20);
    lcd_field(row1, 20);
    lcd_field(row2, 20);
    lcd_field(row3, 20);
}

} // namespace LCD
