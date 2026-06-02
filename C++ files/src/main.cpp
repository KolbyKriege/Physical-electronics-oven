/**
 * @file main.cpp
 * @brief Reflow Oven Controller — Raspberry Pi Pico (C++ / Pico SDK)
 *
 * Direct C++ port of the MicroPython original.
 *
 * State Machine
 * ─────────────
 * Navigation states (encoder rotates between them):
 *   EDIT_TEMP (0) → EDIT_TIME (1) → EDIT_P (2) → EDIT_I (3) → EDIT_D (4) → START (5) → …
 *
 * Button behaviour per state:
 *   EDIT_* : short press toggles EDITING mode for that value;
 *            while editing, encoder changes the value.
 *   START  : short press runs safety checks then enters RUNNING.
 *   RUNNING: short press stops the oven, returns to EDIT_TEMP.
 *
 * Pin assignments
 * ───────────────
 *   GPIO  0  — SSR PWM output
 *   GPIO  2-9 — LCD data D0-D7
 *   GPIO 10   — LCD Enable
 *   GPIO 12   — LCD RS
 *   GPIO 16   — I2C SDA (MCP9600)
 *   GPIO 17   — I2C SCL (MCP9600)
 *   GPIO 19   — Encoder push-button (active LOW, internal pull-up)
 *   GPIO 20   — Encoder DT
 *   GPIO 21   — Encoder CLK
 *   GPIO 25   — Onboard LED (heartbeat)
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "LCD.hpp"
#include "mcp9600.hpp"
#include "pid.hpp"
#include "ssr.hpp"
#include "rotary.hpp"

#include <cstdio>
#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// State constants
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int ST_EDIT_TEMP = 0;
static constexpr int ST_EDIT_TIME = 1;
static constexpr int ST_EDIT_P    = 2;
static constexpr int ST_EDIT_I    = 3;
static constexpr int ST_EDIT_D    = 4;
static constexpr int ST_START     = 5;
static constexpr int ST_RUNNING   = 10;
static constexpr int ST_ERR_TEMP  = 20;
static constexpr int ST_ERR_TIME  = 21;
static constexpr int ST_ERR_PID   = 22;

static const int NAV_STATES[] = {
    ST_EDIT_TEMP, ST_EDIT_TIME, ST_EDIT_P,
    ST_EDIT_I,    ST_EDIT_D,    ST_START
};
static constexpr int NAV_COUNT = 6;

// ─────────────────────────────────────────────────────────────────────────────
// Default user-adjustable parameters
// ─────────────────────────────────────────────────────────────────────────────
static int   set_temp = 500;   // °C
static int   set_time = 60;    // seconds
static float P = 5.0f;
static float I = 1.0f;
static float D = 2.0f;

// Edit step sizes per nav state
static float step_for_state(int s) {
    switch (s) {
        case ST_EDIT_TEMP: return 5.0f;
        case ST_EDIT_TIME: return 10.0f;
        case ST_EDIT_P:    return 0.5f;
        case ST_EDIT_I:    return 0.1f;
        case ST_EDIT_D:    return 0.1f;
        default:           return 1.0f;
    }
}

// Bounds [lo, hi] per nav state
static void bounds_for_state(int s, float& lo, float& hi) {
    switch (s) {
        case ST_EDIT_TEMP: lo =    0; hi = 1000; break;
        case ST_EDIT_TIME: lo =   10; hi = 3600; break;
        case ST_EDIT_P:    lo =    0; hi =   99; break;
        case ST_EDIT_I:    lo =    0; hi =   99; break;
        case ST_EDIT_D:    lo =    0; hi =   99; break;
        default:           lo =    0; hi = 9999; break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Hardware
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint LED_PIN = 25;
static constexpr uint BTN_PIN = 19;

// I2C
static constexpr uint I2C_SDA = 16;
static constexpr uint I2C_SCL = 17;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static float clampf(float val, float lo, float hi) {
    return std::max(lo, std::min(hi, val));
}

static void shutdown_ssr(const char* reason = "") {
    SSR::shutdown();
    if (reason && reason[0]) printf("SHUTDOWN: %s\n", reason);
}

// ─────────────────────────────────────────────────────────────────────────────
// Button debounce
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t BTN_DEBOUNCE_MS = 50;
static int      btn_last_val  = 1;
static uint32_t btn_last_time = 0;
static bool     btn_pressed   = false;

/** Returns true on a clean falling-edge press. */
static bool poll_button() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    int val = gpio_get(BTN_PIN);

    if (val != btn_last_val) {
        btn_last_val  = val;
        btn_last_time = now;
    }
    if (val == 0 && !btn_pressed &&
        (now - btn_last_time) >= BTN_DEBOUNCE_MS)
    {
        btn_pressed = true;
        return true;
    }
    if (val == 1) {
        btn_pressed = false;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Apply encoder delta to the currently-edited value
// ─────────────────────────────────────────────────────────────────────────────
static void apply_encoder_to_value(int delta, int nav_state) {
    float step = step_for_state(nav_state);
    float lo, hi;
    bounds_for_state(nav_state, lo, hi);

    switch (nav_state) {
        case ST_EDIT_TEMP:
            set_temp = (int)clampf((float)set_temp + delta * step, lo, hi);
            break;
        case ST_EDIT_TIME:
            set_time = (int)clampf((float)set_time + delta * step, lo, hi);
            break;
        case ST_EDIT_P:
            P = clampf(P + delta * step, lo, hi);
            break;
        case ST_EDIT_I:
            I = clampf(I + delta * step, lo, hi);
            break;
        case ST_EDIT_D:
            D = clampf(D + delta * step, lo, hi);
            break;
        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    stdio_init_all();

    // ── GPIO ──────────────────────────────────────────────────────────────────
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    // ── I2C ───────────────────────────────────────────────────────────────────
    i2c_init(i2c0, 100000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // ── Peripherals ───────────────────────────────────────────────────────────
    MCP9600      sensor(i2c0);
    RotaryEncoder encoder(/*clk*/21, /*dt*/20);

    SSR::setup(60);   // 60 Hz PWM (matches original)
    LCD::setup();

    PIDController pid(P, I, D, (float)set_temp, 1.0f);

    // ── Runtime state ─────────────────────────────────────────────────────────
    int   state     = ST_EDIT_TEMP;
    int   nav_index = 0;
    bool  editing   = false;
    float curr_temp = sensor.temperature();
    float curr_time = (float)set_time;

    // Safety check lambda
    auto safety_check = [&](int& err_state) -> bool {
        float ambient = sensor.ambient_temperature();
        if ((float)set_temp <= ambient) { err_state = ST_ERR_TEMP; return false; }
        if (set_time <= 0)             { err_state = ST_ERR_TIME; return false; }
        if (P < 0 || I < 0 || D < 0)  { err_state = ST_ERR_PID;  return false; }
        return true;
    };

    auto refresh_lcd = [&]() {
        LCD::update(state, set_temp, curr_temp, set_time, curr_time, P, I, D);
    };

    refresh_lcd();

    printf("Reflow oven controller ready.\n");
    printf("Set temp=%dC  Set time=%ds  P=%.1f I=%.1f D=%.1f\n",
           set_temp, set_time, P, I, D);

    uint32_t last_loop_ms = to_ms_since_boot(get_absolute_time());

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (true) {
        uint32_t now_ms  = to_ms_since_boot(get_absolute_time());
        bool     pressed = poll_button();
        int      delta   = encoder.consume_delta();
        bool     lcd_dirty = false;

        curr_temp = sensor.temperature();

        // Heartbeat LED — toggle every 500 ms
        gpio_put(LED_PIN, (now_ms / 500) % 2 == 0);

        // ════════════════════════════════════════════════════════════════════
        // RUNNING state
        // ════════════════════════════════════════════════════════════════════
        if (state == ST_RUNNING) {
            if (pressed) {
                shutdown_ssr("Stopped by user");
                state     = ST_EDIT_TEMP;
                nav_index = 0;
                editing   = false;
                curr_time = (float)set_time;
                refresh_lcd();
            }
            else if (now_ms - last_loop_ms >= 1000) {
                last_loop_ms = now_ms;
                curr_temp = sensor.temperature();

                float pid_output = pid.update(curr_temp);
                SSR::set_power(pid_output);

                curr_time -= 1.0f;
                lcd_dirty  = true;

                // ── Stop conditions ──────────────────────────────────────
                bool stop = false;
                if (curr_time <= 0.0f) {
                    shutdown_ssr("Time elapsed");
                    stop = true;
                } else if (curr_temp > (float)set_temp + 50.0f) {
                    char buf[40];
                    snprintf(buf, sizeof(buf), "Over-temp: %.0fC", curr_temp);
                    shutdown_ssr(buf);
                    stop = true;
                } else if (sensor.ambient_temperature() > (float)set_temp) {
                    shutdown_ssr("Ambient > setpoint");
                    stop = true;
                }

                if (stop) {
                    state     = ST_EDIT_TEMP;
                    nav_index = 0;
                    editing   = false;
                    curr_time = (float)set_time;
                }
            }

            if (lcd_dirty) refresh_lcd();
        }

        // ════════════════════════════════════════════════════════════════════
        // Error states — any button press dismisses
        // ════════════════════════════════════════════════════════════════════
        else if (state == ST_ERR_TEMP || state == ST_ERR_TIME || state == ST_ERR_PID) {
            if (pressed) {
                state     = ST_EDIT_TEMP;
                nav_index = 0;
                editing   = false;
                refresh_lcd();
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // Navigation / editing states
        // ════════════════════════════════════════════════════════════════════
        else {
            if (pressed) {
                if (state == ST_START) {
                    int err_state = 0;
                    if (!safety_check(err_state)) {
                        state   = err_state;
                        editing = false;
                        refresh_lcd();
                    } else {
                        curr_time = (float)set_time;
                        pid.set_gains(P, I, D);
                        pid.set_setpoint((float)set_temp);
                        pid.reset();
                        last_loop_ms = to_ms_since_boot(get_absolute_time());
                        state   = ST_RUNNING;
                        editing = false;
                        refresh_lcd();
                    }
                } else {
                    editing   = !editing;
                    lcd_dirty = true;
                }
            }

            if (delta != 0) {
                if (editing && state != ST_START) {
                    apply_encoder_to_value(delta, state);
                    lcd_dirty = true;
                } else {
                    nav_index = ((nav_index + delta) % NAV_COUNT + NAV_COUNT) % NAV_COUNT;
                    state     = NAV_STATES[nav_index];
                    editing   = false;
                    lcd_dirty = true;
                }
            }

            if (lcd_dirty) refresh_lcd();
        }

        sleep_ms(20);  // ~50 Hz polling
    }

    return 0; // unreachable
}
