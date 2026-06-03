#include "ssr.hpp"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <algorithm>
#include <cstdio>

static constexpr uint SSR_GPIO = 0;

static uint   pwm_slice  = 0;
static uint   pwm_chan   = 0;
static bool   initialized = false;

namespace SSR {

void setup(uint32_t frequency) {
    gpio_set_function(SSR_GPIO, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(SSR_GPIO);
    pwm_chan  = pwm_gpio_to_channel(SSR_GPIO);

    // Compute wrap value for desired frequency
    // PWM clock = 125 MHz (default sys_clk)
    // freq = sys_clk / (div * (wrap + 1))
    // We use integer divider only for simplicity; adjust if needed.
    uint32_t sys_clk = 125000000;
    uint32_t div     = sys_clk / (frequency * 65536);
    if (div < 1) div = 1;

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, (float)div);
    pwm_config_set_wrap(&cfg, 65535);
    pwm_init(pwm_slice, &cfg, true);

    pwm_set_chan_level(pwm_slice, pwm_chan, 0);
    initialized = true;
}

void set_power(float duty_cycle) {
    if (!initialized) {
        printf("SSR::set_power called before setup()!\n");
        return;
    }
    duty_cycle = std::max(0.0f, std::min(100.0f, duty_cycle));
    uint16_t level = (uint16_t)((duty_cycle / 100.0f) * 65535.0f);
    pwm_set_chan_level(pwm_slice, pwm_chan, level);
}

void shutdown() {
    if (initialized) {
        pwm_set_chan_level(pwm_slice, pwm_chan, 0);
    }
}

} // namespace SSR
