# ESP8266-Telegram

This repository hosts a project which enables the user to remotely control a relay connected to a ESP8266 microcontroller.
The relay can be controlled both from the physical button and a Telegram chat.

I've used Arduino IDE v.2.1.1.

To use this code, you'll need add the following libraries:
- NTPClient by Fabrice Weinberg (3.2.1)
- AsyncTelegram2 by Tolentino Cotesta (2.2.1)
- ArduinoJSON by Benoit Blanchon (6.21.3)

This project has been developped this using an NodeMCU 0.9.

The pins I've used has been as follows:
- D1: Button
- D2: Relay

For each part of this project I've made a dedicated video on my [YouTube channel](https://www.youtube.com/playlist?list=PLkSBHqdar5Xc6R6O29mE0Z6yaiNJBtdgh).

The videos are in Polish but I hope that they'd be helpful when you'd encounter any issues.