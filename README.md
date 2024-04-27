## Firemate IOT
This is the IOT-side code, you can see the full documentation [here](https://github.com/metot-technologies/firemate/blob/main/README.md).

![IOT-shcematic](https://i.ibb.co/5TpHHCW/Untitled-presentation.png)
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
| +                   | 3V3     |
| GND                 | GND     |
#### MQ-2 Smoke/Gas Sensor
| MQ-2 Smoke/Gas Sensor | ESP8266 |
|-----------------------|---------|
| D0                    | D7      |
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

