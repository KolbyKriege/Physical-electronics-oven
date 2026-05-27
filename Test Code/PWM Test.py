from machine import PWM, Pin
import time
LED = Pin(0, Pin.OUT)
#pwm = PWM(LED, freq = 60, duty_u16=0)

while True:
    LED.value(0)
    time.sleep(0.001)
    LED.value(1)
    time.sleep(0.001)