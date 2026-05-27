#system Libraries
import machine
from machine import Pin

import time
#import uasyncio as asyncio
#Custom Libraries
from gpio_lcd import GpioLcd
from rotary_irq_rp2 import RotaryIRQ
import mcp9600
import LCD
import SSR


led = Pin(25, Pin.OUT)

state = 0
r_temp = 1000
r_time = 2000
set_temp = 1000
curr_temp = 0
set_time = 200
curr_time = 200
P = 10
I = 10
D = 10

#mcp9600.setup()

#SSR.setup()
#LCD.update()

Edit_Temp = True
Edit_Time = False
Edit_P = False
Edit_I = False
Edit_D = False
Start = False
Unsafe = True
Button_state = False

#Encoder/Display Setup

#add booleans to control state and whitch states can be shone.
#work through state diagram with smith gray

LCD.setup(set_temp, curr_temp, set_time, curr_time, P, I, D)
r = RotaryIRQ(pin_num_clk=21, pin_num_dt=20, min_val=-1, max_val=1, reverse=False, range_mode=RotaryIRQ.RANGE_WRAP)
r_button = Pin(19,Pin.IN,Pin.PULL_UP)

LCD.update(0,set_temp,curr_temp, set_time,curr_time,P,I,D)

#states = [E_temp,E_time]
r.reset() #sets value back to zero.

val_old = r.value()
while True:
    if r_button.value() == 1:
        Button_state = True
    elif r_button.value() == 0:
        Button_state = False
    
    if r.value() == 0:
        break
    elif r.value() == 1:
        break
    elif r.value() == -1:
        break
    
    if Edit_Temp & Button_state:
        if r.value() == 1:
            curr_temp += 1
        if r.value() == -1:
            curr_temp -= 1


    if val_old != state:
        val_old = state
        print('result =', state)
        LCD.update(state,set_temp,curr_temp, set_time,curr_time,P,I,D)

    time.sleep_ms(50)
    
    
