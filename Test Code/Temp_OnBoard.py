from machine import ADC
from machine import I2C
from machine import Pin
from mcp9600 import MCP9600
import mcp9600

i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
KType = MCP9600(i2c)


def main() -> None:
    sensor = ADC(ADC.CORE_TEMP)
    
    adc_value = sensor.read_u16()
    print(f"adc_value: {adc_value}")
    
    sensor_reading_volts = adc_value * (3.3/65535)
    print(f"sensor_reading: {sensor_reading_volts}")
    
    temperature = 27- (sensor_reading_volts - 0.706)/0.001721
    print(f"Temp: {temperature}\n")
    
    Ktemp = KType.temperature
    print(f"K Temp: {Ktemp}\n")
    
if __name__ == "__main__":
    main()