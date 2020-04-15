# DIY GPS module for Yaesu VX-8DR/VX-8DE Version 3.0

## Overview

This version of the GPS was developed in order to reduce its size and to assemble it in a small plastic enclosure. It is built on a two-layer PCB.

Both schematics and the PCB were developed in KiCAD software. Source files are available in [pcbs/vx8_gps_prototype_v3.0](https://github.com/4x1md/vx8_gps/blob/master/pcbs/vx8_gps_prototype_v3.0) of this repository.

## Schematics

This version uses a G.top015 surface-mounted GPS module based on MTK3333 chipset. It supports both GPS and GLONASS. The module transmits GPS data at 9600 bps. It is very simple to use and doesn't require any software configuration. It uses 3.3V logic levels which match those required by VX-8DR. In order to keep the internal RTC running when the main power is turned off, an external battery can be used. Without it the module will perform a lengthy cold start every time it is powered-on because previous satellite information is not retained and needs to be reacquired. MS621FE backup battery is used in this project.

The microcontroller is ATmega328P like in the previous versions. This version uses the TQFP-32 package.

The circuit is powered from an external 5V source. The power is supplied through a Micro-USB connector. It makes possible to power the GPS from a portable power bank with a USB port. The GPS module and the microcontroller need 3.3V to function. This voltage is supplied by MIC5205-3.3BM5 fixed output linear regulator.

All the passive parts are 1206 SMD for easy soldering. The only through-hole parts are crystal, LEDs and terminal headers.

![Schematic](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_schematic.png)

## Mechanics

One of the requirements of this GPS version was an enclosure box. I chose 51x51x15mm plastic box which is small, strong and can be bought cheap on eBay and AliExpress.

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_01.jpg)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_02.webp)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_03.webp)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_04.webp)

## PCB

The PCB is a two-layer board. Its size is 43.5x43.5mm based on manual measurement of the enclosure box.

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_pcb_0.png)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_pcb_1.jpg)

## Photos

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_pcb_2.jpg)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_pcb_3.jpg)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_pcb_4.jpg)

## Field Tests

The new module was tested in Israel and abroad and I'm satisficed with its performance.

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_tests_1.jpg)

73 from 4X1MD

![73's](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_73.jpg)
