from machine import ADC

def main() -> None:
    sensor = ADC(ADC.CORE_TEMP)
    
    adc_value = sensor.read_u16()
    print(f"adc_value: {adc_value}")
    
    sensor_reading_volts = adc_value * (3.3/65535)
    print(f"sensor_reading: {sensor_reading_volts}")
    
    temperature = 27- (sensor_reading_volts - 0.706)/0.001721
    print(f"Temp: {temperature}\n")
    
    
if __name__ == "__main__":
    main()