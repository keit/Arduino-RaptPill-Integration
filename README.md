## Arduino-RaptPill-Integration
Arduino-RaptPill-Integration uses
[Kegland RAPT Pill Hydrometer & Thermometer](https://kegland.com.au/products/yellow-rapt-pill-hydrometer-thermometer-wifi-bluetooth) to control a heat pad via an Arduino and power relay. The project leverages [REST APIs from rapt.io](https://api.rapt.io/index.html), integrated with an [Arduino Uno R4 WiFi](https://docs.arduino.cc/hardware/uno-r4-wifi/), to monitor the current temperature of a fermentator.

The RAPT Pill sends gravity and temperature data from the fermenter to the [RAPT Cloud](https://app.rapt.io/dashboard) every 30 minutes. The Arduino Uno retrieves this data from the cloud and, if the wort temperature falls below the set threshold, it activates the heating pad.

By default, the temperature threshold is set to 18°C and can be adjusted through a web interface accessible via the Arduino.

### Web Interface
![screen shot](./Screenshot.jpg)
