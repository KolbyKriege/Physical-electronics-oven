from machine import Pin, PWM, I2C
import time
from mcp9600 import MCP9600

# Thermocouple setup
i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
KType = MCP9600(i2c)

# Set up PWM on GPIO0
Bulb = Pin(0, Pin.OUT)
pwm = PWM(Bulb)
pwm.freq(60)
pwm.duty_u16(32536)  # 50%

start_time = time.ticks_ms()

while True:
    elapsed = time.ticks_diff(time.ticks_ms(), start_time)
    Ktemp = KType.temperature
    print(f"K Temp: {Ktemp}\n")

    if elapsed >= 60000:
        print("DONE")

        pwm.duty_u16(0)   # Stop PWM signal
        pwm.deinit()      # Release PWM hardware

        Bulb = Pin(0, Pin.OUT)  # Reinitialize as normal GPIO
        Bulb.value(0)           # Force LOW (OFF)

        break

    time.sleep_ms(10)