#pragma once
#include "hardware/i2c.h"
#include <cstdint>

/**
 * @file mcp9600.hpp
 * @brief MCP9600 thermocouple amplifier I2C driver.
 *
 * Reads hot-junction (thermocouple), cold-junction (ambient), and delta
 * temperatures from a Microchip MCP9600 connected via I2C.
 *
 * Default I2C address: 0x67.
 * Supports thermocouple types: K, J, T, N, S, E, B, R.
 */
class MCP9600 {
public:
    enum class TCType : uint8_t {
        K = 0, J, T, N, S, E, B, R
    };

    /**
     * @param i2c_inst   Pico I2C instance (i2c0 or i2c1).
     * @param address    I2C address (default 0x67).
     * @param type       Thermocouple type (default K).
     * @param tc_filter  Filter coefficient 0-7 (default 0 = off).
     */
    explicit MCP9600(i2c_inst_t* i2c_inst,
                     uint8_t address  = 0x67,
                     TCType   type    = TCType::K,
                     uint8_t  tc_filter = 0);

    /** Hot-junction (thermocouple tip) temperature in °C. */
    float temperature();

    /** Cold-junction (ambient / chip) temperature in °C. */
    float ambient_temperature();

    /** Difference between hot and cold junction in °C. */
    float delta_temperature();

    /** Raw 16-bit device/revision register. */
    uint16_t version();

private:
    i2c_inst_t* i2c;
    uint8_t     addr;

    static constexpr uint8_t REG_HOT_JUNCTION  = 0x00;
    static constexpr uint8_t REG_DELTA_TEMP    = 0x01;
    static constexpr uint8_t REG_COLD_JUNCTION = 0x02;
    static constexpr uint8_t REG_THERM_CFG     = 0x05;
    static constexpr uint8_t REG_VERSION       = 0x20;

    void    write_register(uint8_t reg, uint8_t value);
    int16_t read_raw16(uint8_t reg);
    float   read_temp(uint8_t reg);
};
