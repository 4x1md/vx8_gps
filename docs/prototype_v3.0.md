# DIY GPS module for Yaesu VX-8DR/VX-8DE Version 3.0

## Overview

This version of the GPS was developed in order to reduce its size and to assemble it in a small plastic enclosure. It is built on a two side PCB.

Both schematics and the PCB were developed in KiCAD software. Source files are available in [pcbs/vx8_gps_prototype_v1.0](https://github.com/4x1md/vx8_gps/blob/master/pcbs/vx8_gps_prototype_v3.0) of this repository.

## Schematics

In this version a G.top015 GPS module which is based on MTK3333 chipset. This module is a small size SMT device. It transmits the GPS data at 9600 bps. It is very simple to use and doesn't require any software configuration. It is powered from 3.3V and its supply voltage matches the logic levels required by VX-8DR. In order to keep the internal RTC running when the main power of the GPS module is turned off, an external battery can be used. Without it the GNSS module will perform a lengthy cold start every time it is powered-on because previous satellite information is not retained and needs to be reacquired. MS621FE backup battery is used in this project.

The microcontroller is ATmega328P like in the previous versions. This version uses the TQFP-32 package.

The circuit is powered from an external 5V source. The power is supplied through a Micro-USB connector. It makes possible to power the GPS from a portable power bank with a USB port. The GPS module and the microcontroller need 3.3V to function. This voltage is supplied by MIC5205-3.3BM5 fixed output linear regulator.

All the passive parts are 1206 SMD for easy soldering. The only through-hole parts are crystal, LEDs and terminal headers.

![Schematic](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v3.0_schematic.png)

## Mechanics

One of the requirements of this GPS version is an enclosure box. I chose 51x51x15mm plastic box which is cheap on eBay and AliExpress.

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_01.jpg)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_02.webp)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_03.webp)

![Mechanics](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/mech_04.webp)

## PCB

The PCB is a two layer board. Manual measurement showed that its size must be 43.5x43.5mm in order to fit perfectly the enclosure box. The PCB was manufactured by a Chinese company.

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
