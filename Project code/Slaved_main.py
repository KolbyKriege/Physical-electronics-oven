from machine import Pin, I2C
from mcp9600 import MCP9600
import LCD
import mcp9600
import SSR
import PID
import time

# ============================================
# CONFIGURATION - USER ADJUSTABLE VALUES
# ============================================
set_temp = 500      # Target temperature in degrees C
set_time = 60      # Duration in seconds
P = 5               # PID Proportional gain
I = 1               # PID Integral gain
D = 2               # PID Derivative gain
PWM_FREQ = 60      # PWM frequency in Hz
# ============================================

# Temperature setup
i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
KType = MCP9600(i2c)

# E-Stop button setup
E_Stop = Pin(19, Pin.IN, Pin.PULL_UP)
E_Stop_down = False

# Initialize SSR with PWM
SSR.setup(frequency=PWM_FREQ)

# Initialize PID controller
pid_controller = PID.PIDController(kp=P, ki=I, kd=D, setpoint=set_temp, sample_time=1.0)

# Variables
curr_temp = KType.temperature
curr_time = set_time  # Start from set_time and count down

# Initial LCD display
LCD.setup()

print(f"Starting temperature control:")
print(f"Target Temperature: {set_temp}°C")
print(f"Current Temp: {curr_temp}°C")
print(f"Target Duration: {set_time}s")
print(f"PID Gains - P:{P}, I:{I}, D:{D}")
print(f"PWM Frequency: {PWM_FREQ}Hz")
print()

def shutdown_system(reason=""):
    """Gracefully shutdown the system."""
    print(f"SHUTDOWN: {reason}")
    SSR.shutdown()
    time.sleep(0.1)
    print("SSR powered down. System stopped.")

def safety_check():
    """Check system parameters are valid."""
    if set_temp < KType.ambient_temperature:
        print("ERROR: Target temperature below ambient temperature")
        return False
    if set_time < 0:
        print("ERROR: Set time is negative")
        return False
    if P < 0 or I < 0 or D < 0:
        print("ERROR: PID gains cannot be negative")
        return False
    return True

# Perform safety check
if not safety_check():
    shutdown_system("Safety check failed")
    import sys
    sys.exit()

# Main control loop
try:
    while True:
        # Read current temperature
        curr_temp = KType.temperature
        
        # Update PID controller and get output (0-100% duty cycle)
        pid_output = pid_controller.update(curr_temp)
        
        # Apply PID output to SSR
        SSR.set_power(pid_output)
        
        # Update LCD display
        LCD.update(10, set_temp, curr_temp, set_time, curr_time, P, I, D)
        
        # Check for E-Stop button press
        if E_Stop.value() == False and not E_Stop_down:
            print("E_Stop pressed")
            E_Stop_down = True
            shutdown_system("E-Stop pressed by user")
            break
        
        if E_Stop.value() == True and E_Stop_down:
            E_Stop_down = False
        
        # Decrement time counter
        curr_time -= 1
        
        # Check stopping conditions
        if curr_time <= 0:
            shutdown_system("Time elapsed")
            break
        
        # Safety checks
        if curr_temp > (set_temp + 50):  # Temperature safety margin
            shutdown_system(f"Temperature exceeded setpoint by 50°C: {curr_temp}°C")
            break
        
        if KType.ambient_temperature > set_temp:
            shutdown_system("Ambient temperature exceeded target")
            break
        
        # Wait 1 second before next iteration
        time.sleep(1)

except KeyboardInterrupt:
    print("System interrupted")
    shutdown_system("Keyboard interrupt")
except Exception as e:
    print(f"Error occurred: {e}")
    shutdown_system(f"Exception: {e}")
