## Firemate IOT
This is the IOT-side code, you can see the full documentation [here](https://github.com/metot-technologies/firemate/blob/main/README.md).

![IOT-shcematic](https://i.ibb.co/XCVWC96/schematic.png)

Since the ESP8266 only has one analog pin, but we needed to connect two analog sensors, we implemented a solution to switch the sensors on and off. Here's the process:
- Activate the smoke sensor and deactivate the fire sensor.
- Check for smoke detection with the smoke sensor.
- Deactivate the smoke sensor and activate the fire sensor.
- If the fire sensor detects a flame, it will send the data to Firebase.
### Detailed pin connection
#### NEO-6M GPS Module
| NEO-6M GPS Module | ESP8266 |
|-------------------|---------|
| RX                | D2      |
| TX                | D1      |
| VCC               | VIN     |
| GND               | GND     |
#### KY-026 Flame Sensor
| KY-026 Flame Sensor | ESP8266 |
|---------------------|---------|
| A0                  | A0      |
| +                   | D5      |
| GND                 | GND     |
#### MQ-2 Smoke/Gas Sensor
| MQ-2 Smoke/Gas Sensor | ESP8266 |
|-----------------------|---------|
| A0                    | A0      |
| +                     | 3V3     |
| GND                   | GND     |
#### Buzzer
| Buzzer | ESP8266 |
|--------|---------|
| +      | D6      |
| -      | GND     |
#### ESP32-CAM
| ESP32-CAM | ESP8266 |
|-----------|---------|
| RX        | TX      |
| 3V3       | 3V3     |
| GND       | GND     |

