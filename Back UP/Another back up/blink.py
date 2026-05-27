import machine
import time

Blink = machine.Pin(25, machine.Pin.OUT)

while True:
    Blink.value(1)
    time.sleep(1)
    Blink.value(0)
    time.sleep(1)