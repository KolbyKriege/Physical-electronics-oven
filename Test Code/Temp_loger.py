from machine import Pin, PWM, I2C
import time
from mcp9600 import MCP9600

# Thermocouple setup
i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
KType = MCP9600(i2c)

# Set up PWM on GPIO0
Bulb = Pin(0, Pin.OUT)
Bulb.value(1)
#pwm = PWM(Bulb)
#pwm.freq(60)
#pwm.duty_u16(32536)  # 50%

# Create/open log file and write header
log_file = open("temp_log.csv", "w")
log_file.write("elapsed_ms,temperature_c\n")

start_time = time.ticks_ms()

while True:
    elapsed = time.ticks_diff(time.ticks_ms(), start_time)
    Ktemp = KType.temperature
    print(f"K Temp: {Ktemp}\n")

    # Log to file
    log_file.write(f"{elapsed},{Ktemp}\n")
    log_file.flush()  # Ensure data is written in case of unexpected stop

    if elapsed >= 180000:
        print("DONE")
        #pwm.duty_u16(0)
        #pwm.deinit()
        Bulb = Pin(0, Pin.OUT)
        Bulb.value(0)
        break

    time.sleep_ms(10)

# Close the file cleanly
log_file.close()
print("Log saved to temp_log.csv")