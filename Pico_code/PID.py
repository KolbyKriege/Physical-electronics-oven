# PID controller for temperature regulation

class PIDController:
    """PID controller for temperature regulation."""
    
    def __init__(self, kp=1.0, ki=1.0, kd=1.0, setpoint=0.0, sample_time=1.0):
        """Initialize PID controller.
        
        Args:
            kp (float): Proportional gain (P)
            ki (float): Integral gain (I)
            kd (float): Derivative gain (D)
            setpoint (float): Target temperature
            sample_time (float): Time between samples in seconds (default 1.0)
        """
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.setpoint = setpoint
        self.sample_time = sample_time
        
        # Internal state
        self.prev_error = 0.0
        self.integral = 0.0
        self.last_time = None
        
    def update(self, current_value, current_time=None):
        """Calculate PID output.
        
        Args:
            current_value (float): Current temperature reading
            current_time (float): Current time in seconds (optional, uses sample_time if None)
            
        Returns:
            float: PID output (0.0 to 100.0 for duty cycle)
        """
        # Calculate error
        error = self.setpoint - current_value
        
        # Proportional term
        p_term = self.kp * error
        
        # Integral term (with anti-windup clamping)
        self.integral += error * self.sample_time
        # Clamp integral to prevent windup (roughly proportional to output range)
        self.integral = max(-100.0, min(100.0, self.integral))
        i_term = self.ki * self.integral
        
        # Derivative term
        if current_time is not None:
            dt = current_time - (self.last_time or current_time)
            self.last_time = current_time
        else:
            dt = self.sample_time
            
        if dt > 0:
            derivative = (error - self.prev_error) / dt
        else:
            derivative = 0.0
        d_term = self.kd * derivative
        
        # Update previous error
        self.prev_error = error
        
        # Calculate total output
        output = p_term + i_term + d_term
        
        # Clamp output to 0-100% duty cycle
        output = max(0.0, min(100.0, output))
        
        return output
    
    def set_gains(self, kp, ki, kd):
        """Update PID gains.
        
        Args:
            kp (float): Proportional gain
            ki (float): Integral gain
            kd (float): Derivative gain
        """
        self.kp = kp
        self.ki = ki
        self.kd = kd
    
    def set_setpoint(self, setpoint):
        """Update the target setpoint.
        
        Args:
            setpoint (float): New target temperature
        """
        self.setpoint = setpoint
    
    def reset(self):
        """Reset integral and derivative states."""
        self.prev_error = 0.0
        self.integral = 0.0
        self.last_time = None
