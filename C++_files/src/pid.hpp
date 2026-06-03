#pragma once

/**
 * @file pid.hpp
 * @brief PID controller for temperature regulation.
 *
 * Output is clamped to [0, 100] (percent duty cycle).
 * Includes integral anti-windup clamping at ±100.
 */
class PIDController {
public:
    /**
     * @param kp          Proportional gain.
     * @param ki          Integral gain.
     * @param kd          Derivative gain.
     * @param setpoint    Target temperature (°C).
     * @param sample_time Time between samples in seconds (default 1.0).
     */
    PIDController(float kp = 1.0f, float ki = 1.0f, float kd = 1.0f,
                  float setpoint = 0.0f, float sample_time = 1.0f);

    /**
     * Compute one PID iteration.
     * @param current_value  Current process variable (temperature).
     * @return Output duty cycle in [0, 100].
     */
    float update(float current_value);

    void set_gains(float kp, float ki, float kd);
    void set_setpoint(float setpoint);
    void reset();

private:
    float kp, ki, kd;
    float setpoint;
    float sample_time;

    float prev_error;
    float integral;
};
