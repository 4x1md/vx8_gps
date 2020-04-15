# DIY GPS module for Yaesu VX-8DR/VX-8DE Prototype v1.0

## Overview

One of the aims of the future development of this project is lowering the clock frequency as much as possible. Arduino boards lack the possibility to freely replace the crystal which in this case is necessary. The prototype v1.0 serves as development board providing wider flexibility than Arduino boards. It is built on a dedicated PCB with a standalone ATmega328P-20PU microcontroller in 28 pin DIP package.

Both schematics and the PCB were developed in KiCAD software. Source files are available in [pcbs/vx8_gps_prototype_v1.0](https://github.com/4x1md/vx8_gps/blob/master/pcbs/vx8_gps_prototype_v1.0) of this repository.

## Schematics

The schematics are quite simple. The prototype consists of a microcontroller, an LM317 based voltage regulator and some LEDs for indication purposes.

![Schematic](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_schematic.png)

## PCB

The PCB is designed as two layer board where front layer tracks of the real board are replaced with wire jumpers. It uses some 1206 form factor SMD components.

The board was produced at home using Toner Transfer Method.

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_0.png)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_1.png)

## Photos

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_2.jpg)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_3.jpg)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_4.jpg)

![PCB](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_gps_prototype_v1.0_pcb_5.jpg)

## Links

1. [Cheap and Easy Toner Transfer for PCB Making](http://www.instructables.com/id/Cheap-and-Easy-Toner-Transfer-for-PCB-Making/)
2. [Easy, Consistent & Cheap Toner Transfer Method for Single & Double Sided PCBs](http://www.instructables.com/id/Easy-Consistent-Cheap-Toner-Transfer-Method-for-Si/)


![73's](https://raw.githubusercontent.com/4x1md/vx8_gps/master/docs/images/vx8_73.jpg)
