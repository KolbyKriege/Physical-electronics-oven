#include "rotary.hpp"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ── Full-step state-machine (mirrors MicroPython rotary.py) ──────────────────
//
// Index: current_state[2:0]
// Columns: input (CLK<<1 | DT)   00  01  10  11
//
static const uint8_t TRANSITION_TABLE[8][4] = {
    {0x00, 0x04, 0x01, 0x00},  // R_START (0)
    {0x02, 0x00, 0x01, 0x00},  // R_CW_1  (1)
    {0x02, 0x03, 0x01, 0x00},  // R_CW_2  (2)
    {0x02, 0x03, 0x00, 0x30},  // R_CW_3  (3)  0x30 = START | DIR_CW
    {0x05, 0x04, 0x00, 0x00},  // R_CCW_1 (4)
    {0x05, 0x04, 0x06, 0x00},  // R_CCW_2 (5)
    {0x05, 0x00, 0x06, 0x20},  // R_CCW_3 (6)  0x20 = START | DIR_CCW
    {0x00, 0x00, 0x00, 0x00},  // R_ILLEGAL(7)
};

static constexpr uint8_t STATE_MASK = 0x07;
static constexpr uint8_t DIR_CW     = 0x10;
static constexpr uint8_t DIR_CCW    = 0x20;
static constexpr uint8_t DIR_MASK   = 0x30;

// ── Singleton ─────────────────────────────────────────────────────────────────
RotaryEncoder* RotaryEncoder::instance_ = nullptr;

// ── Constructor ───────────────────────────────────────────────────────────────
RotaryEncoder::RotaryEncoder(uint clk_pin, uint dt_pin)
    : clk_pin(clk_pin), dt_pin(dt_pin),
      pos(0), delta(0), enc_state(0)
{
    instance_ = this;

    gpio_init(clk_pin);
    gpio_set_dir(clk_pin, GPIO_IN);
    gpio_pull_up(clk_pin);

    gpio_init(dt_pin);
    gpio_set_dir(dt_pin, GPIO_IN);
    gpio_pull_up(dt_pin);

    gpio_set_irq_enabled_with_callback(clk_pin,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, &RotaryEncoder::gpio_irq_handler);
    gpio_set_irq_enabled(dt_pin,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

// ── ISR ───────────────────────────────────────────────────────────────────────
void RotaryEncoder::gpio_irq_handler(uint gpio, uint32_t events) {
    (void)gpio; (void)events;
    if (instance_) instance_->process();
}

void RotaryEncoder::process() {
    uint8_t clk_val = gpio_get(clk_pin) ? 1 : 0;
    uint8_t dt_val  = gpio_get(dt_pin)  ? 1 : 0;
    uint8_t pins    = (clk_val << 1) | dt_val;

    enc_state = TRANSITION_TABLE[enc_state & STATE_MASK][pins];

    uint8_t direction = enc_state & DIR_MASK;
    if (direction == DIR_CW) {
        pos++;
        delta++;
    } else if (direction == DIR_CCW) {
        pos--;
        delta--;
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
int RotaryEncoder::value() const {
    return pos.load();
}

void RotaryEncoder::reset() {
    pos.store(0);
    delta.store(0);
}

int RotaryEncoder::consume_delta() {
    return delta.exchange(0);
}
