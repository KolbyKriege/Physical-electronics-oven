#include "mcp9600.hpp"
#include "pico/stdlib.h"
#include <cstdio>
#include <stdexcept>
#include <algorithm>

MCP9600::MCP9600(i2c_inst_t* i2c_inst, uint8_t address, TCType type, uint8_t tc_filter)
    : i2c(i2c_inst), addr(address)
{
    tc_filter = std::min<uint8_t>(tc_filter, 7);
    uint8_t ttype = static_cast<uint8_t>(type);
    uint8_t config = tc_filter | (ttype << 4);
    write_register(REG_THERM_CFG, config);

    // Verify device ID (upper byte of version register must be 0x40 or 0x41)
    uint8_t reg = REG_VERSION;
    uint8_t buf[2] = {0};
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    i2c_read_blocking(i2c, addr, buf, 2, false);
    uint8_t device_id = buf[0];
    if (device_id != 0x40 && device_id != 0x41) {
        // In embedded context we print and halt rather than throwing
        printf("FATAL: MCP9600 not found (id=0x%02X). Check wiring.\n", device_id);
        while (true) { tight_loop_contents(); }
    }
}

void MCP9600::write_register(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(i2c, addr, buf, 2, false);
}

int16_t MCP9600::read_raw16(uint8_t reg) {
    uint8_t buf[2] = {0};
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    i2c_read_blocking(i2c, addr, buf, 2, false);
    // Big-endian signed 16-bit
    return (int16_t)((buf[0] << 8) | buf[1]);
}

float MCP9600::read_temp(uint8_t reg) {
    return read_raw16(reg) * 0.0625f;
}

float MCP9600::temperature() {
    return read_temp(REG_HOT_JUNCTION);
}

float MCP9600::ambient_temperature() {
    return read_temp(REG_COLD_JUNCTION);
}

float MCP9600::delta_temperature() {
    return read_temp(REG_DELTA_TEMP);
}

uint16_t MCP9600::version() {
    uint8_t reg = REG_VERSION;
    uint8_t buf[2] = {0};
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    i2c_read_blocking(i2c, addr, buf, 2, false);
    return (uint16_t)((buf[0] << 8) | buf[1]);
}
