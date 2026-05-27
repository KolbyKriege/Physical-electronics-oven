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
    
    # 0 = blank, 1 = edit tempA, 2 = edit tempB, 3 = edit timeA, 4 = edit timeB, 5 = readyA, 6 = readyB, 7 = not readyA, 8 = not readyB, 9 = StartA, 10 = StartB,
    #11 = StopA, 12 = StopB, 13 = Pa, 14 = Pb, 15 = Ia, 16 = Ib, 17 = Da, 18 = Db,else = blank 
    if state == 0: #Blank state same as the else
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 1: # Edit Temp A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}              P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 2: # Edit Temp B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/        Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 3: # Edit Time A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 4: # Edit Time B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/--:--  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 5: # Ready A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 6: # Ready B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!-----  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 7: # Not Ready A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 8: # Not Ready B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    --- -----  D:{D:02d}")
        
    elif state == 9: # Start A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 10: # Start B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    -----{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 11: # Stop A
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!Ready  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 12: # Stop B
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  -----Ready  P:{P:02d} I:{I:02d}               D:{D:02d}")
        
    elif state == 13: # P a
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 14: # P b
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:-- I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 15: # I a
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 16: # I b
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:--    Not Ready  D:{D:02d}")
        
    elif state == 17: # D a
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:{D:02d}")
        
    elif state == 18: # D b
        lcd.putstr(f"{chr(0)}:{curr_temp:04.0f}/{set_temp:04d}    Start{chr(1)}:{curr_min:02d}:{curr_sec:02d}/{set_min:02d}:{set_sec:02d}  Stop!       P:{P:02d} I:{I:02d}    Not Ready  D:--")
        
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

    
    

