# ESP8266 Bedside Clock

First electronics project. Dense information display for a bedside clock. Time will be synced using WiFi so it's accurate, and the display will be coloured.
Currently run via Micro USB-B, but ideally it will be run using a LiPO battery. The OLED unfortunately draws a lot of power to be run by a battery, so I will need to consider ways to reduce power usage. Could use a button or capacitative sensor to toggle the display, or activate it for a few seconds.

### Devices
- ESP8266 D1 mini clone (Duinotech)
- CCS881 I2C Air quality sensor
- DHT11 Temperature and humidity sensor
- SH1106 SPI 128x64 OLED screen.

### Libraries
- CCS811: https://github.com/maarten-pennings/CCS811
- SSH1106: https://github.com/olikraus/u8g2
- DHT11: https://github.com/adafruit/DHT-sensor-library

Code has somewhat followed the examples available at the above libraries GitHubs.
