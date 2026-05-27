from machine import Pin
from gpio_lcd import GpioLcd
import time

lcd = GpioLcd(rs_pin=Pin(12),
              enable_pin=Pin(10),
              d0_pin=Pin(2),
              d1_pin=Pin(3),
              d2_pin=Pin(4),
              d3_pin=Pin(5),
              d4_pin=Pin(6),
              d5_pin=Pin(7),
              d6_pin=Pin(8),
              d7_pin=Pin(9),
              num_lines = 4, num_columns = 20)
temp_char = [0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x11, 0x0E]
time_char = [0x00, 0x00, 0x0E, 0x13, 0x15, 0x11, 0x0E, 0x00]
prev_state = 0


def setup(set_temp,curr_temp, set_time,curr_time,P,I,D):
    
    
    set_min, set_sec = divmod(set_time, 60)
    curr_min, curr_sec = divmod(curr_time, 60)
    
    lcd.clear()
    lcd.custom_char(0,temp_char)
    lcd.custom_char(1,time_char)
    lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
    
def update(state,set_temp,curr_temp, set_time,curr_time,P,I,D):
    lcd.clear()
    
    set_min, set_sec = divmod(set_time, 60)
    curr_min, curr_sec = divmod(curr_time, 60)
    
    # 0 = blank, 1 = Runing Mode, 2 = temp, 3 = time, P =, I =, D =, 4= Start,  else = blank 
    if state == 0: #Blank state same as the else
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
    
    elif state == 1: # Running Mode
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}         {chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       P:{P:02d} I:{I:02d} D:{D:02d}      Press Encoder 2 STOP")
        
    elif state == 2: # Edit Temp 
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}<   Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       P:{P:02d}  I:{I:02d}  D:{D:02d}")
        
    elif state == 3: # Edit Time
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/--:--<      P:{P:02d}  I:{I:02d}  D:{D:02d}")
            
    elif state == 4: # Start 
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}   >Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       P:{P:02d}  I:{I:02d}  D:{D:02d}")
                    
    elif state == 5: # P 
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       >P:{P:02d}  I:{I:02d}  D:{D:02d}")
            
    elif state == 6: # I 
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       P:{P:02d} >I:{I:02d}  D:{D:02d}")
            
    elif state == 7: # D 
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}       P:{P:02d}  I:{I:02d} >D:{D:02d}")
            
    elif state == 8: # Temperature Error 
        lcd.putstr(f"ERROR: Target temperature below ambient temperature")

    elif state == 9: # Time Error 
        lcd.putstr(f"ERROR: Set time is  negative")
    
    elif state == 10: # PID Error 
        lcd.putstr(f"ERROR: PID gains cannot be negative")
            
    else:
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")


def State_determination(state,set_temp,curr_temp, set_time,curr_time,P,I,D):
    
    if state > prev_state:
        prev_state += 1
    if state < prev_state:
        prev_state -= 1
    
    if prev_state == 1:
        LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)
    elif prev_state == 2:
        LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)
    elif prev_state == 3:    
        LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)
    elif prev_state == 4:
        LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)
    elif prev_state == 5:
        LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)
    
    #LCD.update(prev_state,set_temp,curr_temp, set_time,curr_time,P,I,D)

    
    
