#include "pid.hpp"
#include <algorithm>

PIDController::PIDController(float kp, float ki, float kd,
                             float setpoint, float sample_time)
    : kp(kp), ki(ki), kd(kd),
      setpoint(setpoint), sample_time(sample_time),
      prev_error(0.0f), integral(0.0f)
{}

float PIDController::update(float current_value) {
    float error = setpoint - current_value;

    // Proportional
    float p_term = kp * error;

    // Integral with anti-windup
    integral += error * sample_time;
    integral  = std::max(-100.0f, std::min(100.0f, integral));
    float i_term = ki * integral;

    // Derivative (backward difference over sample_time)
    float derivative = (sample_time > 0.0f)
                       ? (error - prev_error) / sample_time
                       : 0.0f;
    float d_term = kd * derivative;

    prev_error = error;

    float output = p_term + i_term + d_term;
    return std::max(0.0f, std::min(100.0f, output));
}

void PIDController::set_gains(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PIDController::set_setpoint(float setpoint) {
    this->setpoint = setpoint;
}

void PIDController::reset() {
    prev_error = 0.0f;
    integral   = 0.0f;
}
