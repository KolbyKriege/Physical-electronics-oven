"""LCD display module for the reflow oven controller.

Handles all display formatting for a 20x4 character LCD.
All state logic lives in main.py - this module only renders.

Display layout (20 cols x 4 rows):
  Row 0: [T]CCCC/SSSS  XXXXX   (temp: current/set, right side = status/cursor)
  Row 1: [C]MM:SS/MM:SS  XXXXX  (time: current/set, right side = status/cursor)
  Row 2: P:PP  I:II  D:DD    (PID values)
  Row 3: context line           (mode info / errors)
"""

from machine import Pin
from gpio_lcd import GpioLcd

# Custom character bitmaps
_TEMP_CHAR = [0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x11, 0x0E]  # thermometer
_TIME_CHAR = [0x00, 0x00, 0x0E, 0x13, 0x15, 0x11, 0x0E, 0x00]  # clock

lcd = GpioLcd(
    rs_pin=Pin(12),
    enable_pin=Pin(10),
    d0_pin=Pin(2),
    d1_pin=Pin(3),
    d2_pin=Pin(4),
    d3_pin=Pin(5),
    d4_pin=Pin(6),
    d5_pin=Pin(7),
    d6_pin=Pin(8),
    d7_pin=Pin(9),
    num_lines=4,
    num_columns=20
)


def setup():
    """Load custom characters. Call once at startup."""
    lcd.custom_char(0, _TEMP_CHAR)
    lcd.custom_char(1, _TIME_CHAR)


def _fmt_time(seconds):
    """Format seconds into MM:SS string."""
    m, s = divmod(max(0, int(seconds)), 60)
    return f"{m:02d}:{s:02d}"


def update(state, set_temp, curr_temp, set_time, curr_time, P, I, D):
    """Redraw the entire LCD for the given state.

    States:
      0  = EDIT_TEMP   (cursor on temp)
      1  = EDIT_TIME   (cursor on time)
      2  = EDIT_P      (cursor on P)
      3  = EDIT_I      (cursor on I)
      4  = EDIT_D      (cursor on D)
      5  = START       (cursor on Start)
      10 = RUNNING
      20 = ERROR_TEMP
      21 = ERROR_TIME
      22 = ERROR_PID
    """
    ct = _fmt_time(curr_time)
    st = _fmt_time(set_time)

    # Build cursor markers - '>' means this field is selected/active
    t_cur  = "<" if state == 0  else " "
    tm_cur = "<" if state == 1  else " "
    p_cur  = ">" if state == 2  else " "
    i_cur  = ">" if state == 3  else " "
    d_cur  = ">" if state == 4  else " "
    s_cur  = "<" if state == 5  else " "

    # ---- Error screens (full display) ----
    if state == 20:
        lcd.clear()
        lcd.putstr("ERROR:Temp too low  "
                   "Target below ambient"
                   "                    "
                   "Fix set temp & retry")
        return

    if state == 21:
        lcd.clear()
        lcd.putstr("ERROR: Invalid time "
                   "Set time must be > 0"
                   "                    "
                   "Fix set time & retry")
        return

    if state == 22:
        lcd.clear()
        lcd.putstr("ERROR: Bad PID gains"
                   "P, I, D must be >= 0"
                   "                    "
                   "Fix PID & retry     ")
        return

    # ---- Running screen ----
    if state == 10:
        lcd.clear()
        lcd.putstr(
            f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}         "   # row 0 (20)
            f"{chr(1)}:{ct}/{st}       "                          # row 1 (20)
            f"P:{P:.1f} I:{I:.1f} D:{D:.1f}   "                    # row 2 (20)
            f"Press Encoder 2 STOP"                               # row 3 (20)
        )
        return

    # ---- Normal navigation / edit screens ----
    lcd.clear()
    lcd.putstr(
        f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}{t_cur}        "  # row 0 (20)
        f"{chr(1)}:{ct}/{st}{tm_cur}      "                          # row 1 (20)
        f"{p_cur}P:{P:.1f} {i_cur}I:{I:.1f} {d_cur}D:{D:.1f}"                          # row 2 (20)
        f"START{s_cur}   "               # row 3 (20)
    )
