from machine import I2C
import struct

_DEFAULT_ADDRESS = 0x67

_REGISTER_HOT_JUNCTION = 0x00
_REGISTER_DELTA_TEMP = 0x01
_REGISTER_COLD_JUNCTION = 0x02
_REGISTER_THERM_CFG = 0x05
_REGISTER_VERSION = 0x20


class MCP9600:

    TYPES = ("K", "J", "T", "N", "S", "E", "B", "R")

    def __init__(self, i2c: I2C, address=_DEFAULT_ADDRESS, tctype="K", tcfilter=0):
        self.i2c = i2c
        self.address = address

        if tctype not in self.TYPES:
            raise ValueError("Invalid thermocouple type")

        tcfilter = max(0, min(7, tcfilter))
        ttype = self.TYPES.index(tctype)

        # Configure thermocouple type + filter
        config_value = tcfilter | (ttype << 4)
        self._write_register(_REGISTER_THERM_CFG, bytes([config_value]))

        # Verify device ID
        device_id = self._read_device_id()
        if device_id not in (0x40, 0x41):
            raise RuntimeError("MCP9600 not found. Check wiring.")

    # ------------------------
    # Low Level I2C Functions
    # ------------------------

    def _write_register(self, reg, data):
        self.i2c.writeto_mem(self.address, reg, data)

    def _read_register(self, reg, length):
        return self.i2c.readfrom_mem(self.address, reg, length)

    def _read_device_id(self):
        data = self._read_register(_REGISTER_VERSION, 2)
        # Device ID is upper byte
        return data[0]

    # ------------------------
    # Temperature Properties
    # ------------------------

    @property
    def temperature(self):
        return self._read_temp(_REGISTER_HOT_JUNCTION)

    @property
    def ambient_temperature(self):
        return self._read_temp(_REGISTER_COLD_JUNCTION)

    @property
    def delta_temperature(self):
        return self._read_temp(_REGISTER_DELTA_TEMP)

    def _read_temp(self, reg):
        data = self._read_register(reg, 2)

        raw = struct.unpack(">h", data)[0]  # signed 16-bit
        return raw * 0.0625

    @property
    def version(self):
        data = self._read_register(_REGISTER_VERSION, 2)
        return struct.unpack(">H", data)[0]