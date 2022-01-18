EasyRider
=========

This project is frozen. It's here for archival/educational purposes.

_UPDATE:_ EasyRider REV C is a completely new project, it is based on
a way more powerful (32bit instead of 8bit) STM32 Arm Cortex M4 microcontroller.

EasyRider REV B is a motorcycle telemetrics project, developed specifically for my Honda C90 classic bike.
It uses more advanced code and SMD components than Rev A.

The software is written in bare-metal C for the Atmel AVR ATmega1284p 8bit microcontroller.

The PCB was created with Altium, you'll find the gerbers/schematics/datasheets in the /docs directory.

![Board](https://github.com/stamina/easyriderb/raw/main/images/easyrider_revb.jpg "Board")
![Board](https://github.com/stamina/easyriderb/raw/main/images/easyrider_3d_altium.png "Board")

Functionality
-------------

EasyRider basically does the following things:

- Controls the lighting system: indicators, rear light, brake light, etc.
- Controls the claxon
- Contains a simple alarm system, based on an accelerometer
- Reads the motorcycle battery voltage and the power draw of the board
- Contains a little buzzer for some startup/alarm sounds
- Reads the RPM of the motor via the CDI, through an opamp and optocoupled circuit
- Logs GPS coordinates and timestamps of events via the external RTC
- Sends all telemetry data over Wifi or writes it to the microSD card

Notable components
------------------
2x Atmel AVR ATmega1284P microcontrollers
1x Accelerometer MMA7361L board
1x GPS Venus638FLPx sparkfun board
1x WiFly-RN-XV-DS sparkfun board
1x lm35 temperature sensor
9x Meder reed relays CRR05-1A
1x Opamp tlv2371
1x Optocoupler vo610a
1x Real Time Clock DS1307.pdf
2x uln2003lv transistor arrays


