#lib for interacting with SSRs
from machine import Pin, PWM

# SSR is controlled via GPIO 0 with PWM
ssr_pin = Pin(0, Pin.OUT)
ssr_pwm = None

def setup(frequency=500):
    """Initialize PWM on GPIO 0 for SSR control.
    
    Args:
        frequency (int): PWM frequency in Hz (default 500Hz)
    """
    global ssr_pwm
    ssr_pwm = PWM(ssr_pin)
    ssr_pwm.freq(frequency)
    ssr_pwm.duty_u16(0)  # Start with 0% duty cycle

def set_power(duty_cycle):
    """Set the SSR power level via PWM duty cycle.
    
    Args:
        duty_cycle (float): Duty cycle as percentage (0.0 to 100.0)
    """
    if ssr_pwm is None:
        raise RuntimeError("SSR not initialized. Call setup() first.")
    
    # Clamp duty cycle between 0 and 100
    duty_cycle = max(0.0, min(100.0, duty_cycle))
    
    # Convert percentage (0-100) to u16 (0-65535)
    duty_u16 = int((duty_cycle / 100.0) * 65535)
    ssr_pwm.duty_u16(duty_u16)

def shutdown():
    """Turn off the SSR completely."""
    if ssr_pwm is not None:
        ssr_pwm.duty_u16(0)