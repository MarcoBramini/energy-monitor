# EnergyMonitor

An ESP32 iot-based application that enables to measure the power consumption of an house.
This is designed to work within HomeAssistant and requires the installation of a MQTT server.

The application automatically sends discovery messages and, therefore, automatically configure itself in HomeAssistant.

The required hardware is:
- ESP32
- SCT-013 30a/1v Clamp Current Sensor
- Voltage step-down buck converter

The exposed devices are:
- energy_meter_current_sensor
- energy_meter_power_sensor

