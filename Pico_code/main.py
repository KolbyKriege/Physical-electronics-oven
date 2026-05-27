"""Reflow Oven Controller - Main Entry Point
Runs standalone on a Raspberry Pi Pico 2.

State Machine
─────────────
Navigation states (encoder rotates between them):
  EDIT_TEMP  (0)  ──encoder──>  EDIT_TIME (1) ──> EDIT_P (2)
  EDIT_I     (3)  ──encoder──>  EDIT_D    (4) ──> START  (5)
  ... wraps back to EDIT_TEMP

Button behaviour per state:
  EDIT_TEMP / EDIT_TIME / EDIT_P / EDIT_I / EDIT_D:
      Short press  → enter EDITING mode for that value
      While editing, encoder changes the value; press again to exit editing

  START:
      Short press  → run safety checks, then enter RUNNING (10)

  RUNNING (10):
      Short press  → stop, return to EDIT_TEMP

Pin assignments
───────────────
  GPIO 0       SSR PWM output
  GPIO 2-9     LCD data (D0-D7)
  GPIO 10      LCD Enable
  GPIO 12      LCD RS
  GPIO 16      I2C SDA (MCP9600)
  GPIO 17      I2C SCL (MCP9600)
  GPIO 19      Encoder push button (active LOW, internal pull-up)
  GPIO 20      Encoder DT
  GPIO 21      Encoder CLK
  GPIO 25      Onboard LED (heartbeat)
"""

from machine import Pin, I2C
from rotary_irq_rp2 import RotaryIRQ
from mcp9600 import MCP9600
import LCD
import SSR
import PID
import time

# ─────────────────────────────────────────
# Default user-adjustable values
# ─────────────────────────────────────────
set_temp  = 500   # °C
set_time  = 60    # seconds
P         = 5
I         = 1
D         = 2
PWM_FREQ  = 60    # Hz

# ─────────────────────────────────────────
# State constants
# ─────────────────────────────────────────
ST_EDIT_TEMP = 0
ST_EDIT_TIME = 1
ST_EDIT_P    = 2
ST_EDIT_I    = 3
ST_EDIT_D    = 4
ST_START     = 5
ST_RUNNING   = 10
ST_ERR_TEMP  = 20
ST_ERR_TIME  = 21
ST_ERR_PID   = 22

NAV_STATES = [ST_EDIT_TEMP, ST_EDIT_TIME, ST_EDIT_P,
              ST_EDIT_I,    ST_EDIT_D,    ST_START]

# Value edit step sizes per state
STEP = {
    ST_EDIT_TEMP: 5,   # °C per click
    ST_EDIT_TIME: 10,  # seconds per click
    ST_EDIT_P:    0.5,
    ST_EDIT_I:    0.1,
    ST_EDIT_D:    0.1,
}

# Value bounds [min, max]
BOUNDS = {
    ST_EDIT_TEMP: (0,   1000),
    ST_EDIT_TIME: (10,  3600),
    ST_EDIT_P:    (0,   99),
    ST_EDIT_I:    (0,   99),
    ST_EDIT_D:    (0,   99),
}

# ─────────────────────────────────────────
# Hardware init
# ─────────────────────────────────────────
led = Pin(25, Pin.OUT)

i2c  = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
sensor = MCP9600(i2c)

SSR.setup(frequency=PWM_FREQ)

r = RotaryIRQ(
    pin_num_clk=21,
    pin_num_dt=20,
    min_val=0,
    max_val=100,          # large range; we track delta manually
    reverse=False,
    range_mode=RotaryIRQ.RANGE_UNBOUNDED,
    pull_up=True
)
r.reset()

btn = Pin(19, Pin.IN, Pin.PULL_UP)

LCD.setup()

# ─────────────────────────────────────────
# Runtime state
# ─────────────────────────────────────────
state        = ST_EDIT_TEMP
nav_index    = 0           # index into NAV_STATES for navigation
editing      = False       # True when we're editing a value in the current state
curr_temp    = 0.0
curr_time    = set_time

pid_controller = PID.PIDController(kp=P, ki=I, kd=D,
                                   setpoint=set_temp, sample_time=1.0)

encoder_last = r.value()

# Button debounce
BTN_DEBOUNCE_MS = 50
btn_last_val    = 1
btn_last_time   = 0
btn_pressed     = False    # single-event flag, consumed once per press

# Timer tracking for 1 Hz loop
last_loop_ms = time.ticks_ms()


# ─────────────────────────────────────────
# Helper functions
# ─────────────────────────────────────────

def clamp(val, lo, hi):
    return max(lo, min(hi, val))


def shutdown_ssr(reason=""):
    SSR.shutdown()
    if reason:
        print("SHUTDOWN:", reason)


def safety_check():
    """Return (ok, error_state). Reads ambient temperature live."""
    ambient = sensor.ambient_temperature
    if set_temp <= ambient:
        return False, ST_ERR_TEMP
    if set_time <= 0:
        return False, ST_ERR_TIME
    if P < 0 or I < 0 or D < 0:
        return False, ST_ERR_PID
    return True, None


def read_encoder_delta():
    """Return how many detents the encoder moved since last call."""
    global encoder_last
    now = r.value()
    delta = now - encoder_last
    encoder_last = now
    return delta


def poll_button():
    """Debounced button edge detector. Returns True on a falling edge (press)."""
    global btn_last_val, btn_last_time, btn_pressed
    now = time.ticks_ms()
    val = btn.value()
    if val != btn_last_val:
        btn_last_val = val
        btn_last_time = now
    if (val == 0
            and not btn_pressed
            and time.ticks_diff(now, btn_last_time) >= BTN_DEBOUNCE_MS):
        btn_pressed = True
        return True
    if val == 1:
        btn_pressed = False
    return False


def apply_encoder_to_value(delta, nav_state):
    """Apply encoder delta to whichever value nav_state controls."""
    global set_temp, set_time, P, I, D
    step = STEP.get(nav_state, 1)
    lo, hi = BOUNDS.get(nav_state, (0, 9999))
    if nav_state == ST_EDIT_TEMP:
        set_temp = clamp(set_temp + delta * step, lo, hi)
    elif nav_state == ST_EDIT_TIME:
        set_time = clamp(set_time + delta * step, lo, hi)
    elif nav_state == ST_EDIT_P:
        P = clamp(P + delta * step, lo, hi)
    elif nav_state == ST_EDIT_I:
        I = clamp(I + delta * step, lo, hi)
    elif nav_state == ST_EDIT_D:
        D = clamp(D + delta * step, lo, hi)


def refresh_lcd():
    """Push current state to the display."""
    LCD.update(state, set_temp, curr_temp, set_time, curr_time, P, I, D)


# ─────────────────────────────────────────
# Initial display
# ─────────────────────────────────────────
curr_temp = sensor.temperature
refresh_lcd()

print("Reflow oven controller ready.")
print(f"Set temp={set_temp}C  Set time={set_time}s  P={P} I={I} D={D}")

# ─────────────────────────────────────────
# Main loop
# ─────────────────────────────────────────
while True:
    now_ms   = time.ticks_ms()
    pressed  = poll_button()
    delta    = read_encoder_delta()
    lcd_dirty = False
    
    curr_temp = sensor.temperature
    
    # ── LED heartbeat ─────────────────────
    led.value(1 if (now_ms // 500) % 2 == 0 else 0)

    # ══════════════════════════════════════
    # RUNNING state
    # ══════════════════════════════════════
    if state == ST_RUNNING:

        # Button stops the oven
        if pressed:
            shutdown_ssr("Stopped by user")
            state     = ST_EDIT_TEMP
            nav_index = 0
            editing   = False
            curr_time = set_time
            refresh_lcd()

        # 1 Hz control loop
        elif time.ticks_diff(now_ms, last_loop_ms) >= 1000:
            last_loop_ms = now_ms

            curr_temp = sensor.temperature

            # PID → SSR
            pid_output = pid_controller.update(curr_temp)
            SSR.set_power(pid_output)

            curr_time -= 1
            lcd_dirty  = True

            # ── Stopping conditions ──────────
            if curr_time <= 0:
                shutdown_ssr("Time elapsed")
                state = ST_EDIT_TEMP
                nav_index = 0
                curr_time = set_time
                editing = False

            elif curr_temp > set_temp + 50:
                shutdown_ssr(f"Over-temp: {curr_temp:.0f}C")
                state = ST_EDIT_TEMP
                nav_index = 0
                curr_time = set_time
                editing = False

            elif sensor.ambient_temperature > set_temp:
                shutdown_ssr("Ambient > setpoint")
                state = ST_EDIT_TEMP
                nav_index = 0
                curr_time = set_time
                editing = False

            if lcd_dirty:
                refresh_lcd()

    # ══════════════════════════════════════
    # Error states — button to dismiss
    # ══════════════════════════════════════
    elif state in (ST_ERR_TEMP, ST_ERR_TIME, ST_ERR_PID):
        if pressed:
            state     = ST_EDIT_TEMP
            nav_index = 0
            editing   = False
            refresh_lcd()

    # ══════════════════════════════════════
    # Navigation / editing states
    # ══════════════════════════════════════
    else:
        
        # ── Button behaviour ──────────────
        if pressed:
            if state == ST_START:
                # Run safety check then start
                ok, err_state = safety_check()
                if not ok:
                    state   = err_state
                    editing = False
                    refresh_lcd()
                else:
                    # Start the oven
                    curr_time = set_time
                    pid_controller.set_gains(P, I, D)
                    pid_controller.set_setpoint(set_temp)
                    pid_controller.reset()
                    last_loop_ms = time.ticks_ms()
                    state   = ST_RUNNING
                    editing = False
                    refresh_lcd()
            else:
                # Toggle editing mode for current nav state
                editing = not editing
                lcd_dirty = True

        # ── Encoder behaviour ─────────────
        if delta != 0:
            if editing and state != ST_START:
                # Change the value assigned to this nav state
                apply_encoder_to_value(delta, state)
                lcd_dirty = True
            else:
                # Navigate between states
                nav_index = (nav_index + delta) % len(NAV_STATES)
                state     = NAV_STATES[nav_index]
                editing   = False
                lcd_dirty = True

        if lcd_dirty:
            refresh_lcd()

    time.sleep_ms(20)   # ~50 Hz polling, keeps ISR breathing room
