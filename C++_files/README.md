# Reflow Oven Controller — Raspberry Pi Pico (C++)

C++ port of the MicroPython reflow oven controller, targeting the
Raspberry Pi Pico / Pico 2 using the official **Pico SDK**.

## Project structure

```
C++ Files/
├── CMakeLists.txt
└── src/
    ├── main.cpp           # State machine & main loop
    ├── lcd_api.hpp/.cpp   # HD44780 base class (hardware-agnostic)
    ├── gpio_lcd.hpp/.cpp  # GPIO-driven HD44780 driver
    ├── LCD.hpp/.cpp       # High-level 20×4 display module
    ├── mcp9600.hpp/.cpp   # MCP9600 thermocouple amplifier I2C driver
    ├── pid.hpp/.cpp       # PID controller
    ├── ssr.hpp/.cpp       # Solid State Relay PWM driver
    └── rotary.hpp/.cpp    # Interrupt-driven rotary encoder
```

## Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) ≥ 1.5
- CMake ≥ 3.13
- ARM GCC toolchain (`arm-none-eabi-gcc`)

Set the environment variable:
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Flash `reflow_oven.uf2` to the Pico by holding BOOTSEL and dragging the
file onto the mass-storage drive.

## Pin assignments

| GPIO | Function            |
|------|---------------------|
|  0   | SSR PWM output      |
|  2   | LCD D0              |
|  3   | LCD D1              |
|  4   | LCD D2              |
|  5   | LCD D3              |
|  6   | LCD D4              |
|  7   | LCD D5              |
|  8   | LCD D6              |
|  9   | LCD D7              |
| 10   | LCD Enable          |
| 12   | LCD RS              |
| 16   | I2C0 SDA (MCP9600)  |
| 17   | I2C0 SCL (MCP9600)  |
| 19   | Encoder button (active LOW) |
| 20   | Encoder DT          |
| 21   | Encoder CLK         |
| 25   | Onboard LED (heartbeat) |

## Porting notes

| Topic | Detail |
|---|---|
| **GpioLcd constructor** | The MicroPython version re-ran `LcdApi.__init__` inside `GpioLcd.__init__`. In C++ the base constructor runs first, so the LCD is initialised in two stages: base-class cmds issue during `LcdApi()`, then the `GpioLcd` ctor sends the function-set and 2-line commands. |
| **Rotary encoder** | The MicroPython original uses RP2-specific IRQ handlers. Replaced with a Pico SDK `gpio_set_irq_enabled_with_callback` pattern and a `std::atomic` delta counter consumed each loop tick via `consume_delta()`. Only one encoder instance is supported (ISR singleton). |
| **SSR PWM** | MicroPython `PWM.duty_u16()` maps directly to `pwm_set_chan_level()` with a 16-bit wrap. The divider is calculated from `sys_clk / (freq × 65536)`. |
| **MCP9600** | Throws (halts) in the constructor if the device ID does not match, matching the MicroPython `RuntimeError`. |
| **PID** | Identical algorithm; `current_time` optional parameter removed — the fixed `sample_time` is always used, matching actual loop behaviour. |
| **LCD formatting** | `snprintf` + `lcd_field()` (pads/truncates to exactly N chars) replaces Python f-strings. Custom chars 0 and 1 are still `\x00` / `\x01`. |
