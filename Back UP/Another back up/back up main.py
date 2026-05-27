#back up main
#system Libraries
import machine
from machine import Pin

import time
import uasyncio as asyncio
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
curr_temp = 1000
set_time = 200
curr_time = 200
P = 10
I = 10
D = 10

#mcp9600.setup()
LCD.setup(set_temp,curr_temp, set_time,curr_time,P,I,D)
#SSR.setup()
#LCD.update()



#Encoder/Display Setup
async def update_display(delay):
    while True:
        LCD.update(state,set_temp,curr_temp, set_time,curr_time,P,I,D)
        await asyncio.sleep(delay)

event = asyncio.Event()
def callback():
    event.set()
r = RotaryIRQ(pin_num_clk=21,pin_num_dt=20,min_val=0,max_val=10,)
r.add_listener(callback)

async def main():
    asyncio.create_task(update_display(0.3))

    while True:
        await event.wait()
        # update the global state so the display task sees changes
        global state
        state = r.value()
        print('result =', state)
        event.clear()


asyncio.run(main()) #must be at the end, is what ensureproper running of asyncio functions

    
    
    
    
