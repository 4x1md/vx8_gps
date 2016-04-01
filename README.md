# DIY GPS module for Yaesu VX-8DR/VX-8DE handheld transceiver
GPS module for Yaesu VX-8DR/DE handheld transceivers with ublox NEO-6M chip and Arduino Nano by Dmitry Melnichansky 4Z7DTF.

## Overview

When I was buying my Yaesu VX-8DE, I knew that I wasn't going to buy the original Yaesu FGPS2 module. There is very little APRS coverage in Israel and APRS in general seemed quite useless to me. The unreasonably high price of the original module didn't make me want to try it either.

After receiving far APRS stations during a strong tropo over the Mediterranean I thought it would be nice to transmit some data. I started investigating how GPS transmits data and discovered the NMEA protocol I had never encountered before. My first source of information was the article [Reverse Engineering the Yaesu VX-8DR GPS Interface](http://lingnik.com/2013/02/09/reverse-engineering-yaesu-vx-8dr-gps-interface.html) by Taylor J Meek KG7BBG.

I started from writing a Python program which was sending NMEA sentences to the transceiver. After some research I discovered the required message format and wrote a program which drew flowers on APRS map.

![APRS flower](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/aprs_flower.png)

Later, [David Fannin's project](https://github.com/dfannin/arduino-vx8r-gps) inspired me to build my own GPS module. Thanks to his project I found out the ublox NEO-6M chip and a simple search on AliExpress showed that a ready module can be puchased for $10-20.

I wrote my own software which isn't based on David Fannin's code. I started it as Arduino project but after finding out that the standard Arduino Serial library doesn't support interrupt driven writing and reading I decided to write it in pure C. After small changes the C code can be compiled in Arduino environment. The Arduino code is located in [/arduino/vx8_gps_16mhz folder](https://github.com/4z7dtf/vx8_gps/blob/master/arduino/vx8_gps_16mhz) of this repository.

I tried to document this project as much as possible and I hope it will be interesting and useful for other radio amateurs.

## Yaesu VX-8DR/DE and NMEA protocol
The NMEA protocol is implemented in the VX-8 in a non-standard way. There are two main issues with it: serial port parameters and NMEA sentence format. There is also a data validation issue which is very interesting but less important in this case.

### Port parameters: baud rate and logic levels
The original Yaesu FGPS2 module transmits at 9600 bps instead of the standard 4800 bps and uses 3.3V logic level instead of the standard 5V. This isn't a big issue because there are GPS modules which can be configured to transmit at 9600 bps and logic levels can be converted. The GY-GPS6MV2 module came already pre-configured to 9600 bps rate. The ublox NEO-6M chip used in this module has maximum supply voltage of 3.6V. As we see there are no electrical compatibility issues at all.
### NMEA sentence format: field lengths
VX-8 requires all the fields of the NMEA sentence to be padded to the maximum possible number of symbols while regular NMEA standart allows fields to be empty. It means that if there is no GPS fix, typical GGA message will have the following form:
```
$GPGGA,074222.000,,,,,0,00,99.9,,,,,,0000*6E
```
The radio won't be able to process this message and we'll see gibberish on the display. A correct sentence for this transceiver should have the following format:
```
$GPGGA,200124.000,0000.0000,N,00000.0000,E,0,00,00.0,00000.0,M,0000.0,M,000.0,0000*46
```
Sending a sentence with empty fields will result in gibberish on the GPS screen of VX-8. Here is what we'll get.

![Yaesu VX-8DE](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_0.jpg)

Allthough the sentence accords with the standard, the transceiver isn't able to process it correctly.

Actually setting number of sybols in each field is what all this project is about! If the second issue didn't exist we could connect the ublox NEO-6M chip based module directly to the transceiver.

### NMEA data validation
Here the things get even more interesting. Yaesu VX-8 does no validation on the NMEA data. When I say no validation, I mean **no validation at all**! Let's look at the following NMEA sentence.
```
$GPGGA,999999.000,73DE.0000,!,Z7DTF.NOPQ,4,1,04
```
The time in the first field is invalid, the latitude and longitude fields have letters instead of numbers, the North/South and East/West fields have invalid characters as well. The checksum is missing. Should it be thrown away by the receiving device? Yes, sure! But let's see how VX-8 will respond!

![Yaesu VX-8DE](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_1a.jpg)

![Yaesu VX-8DE](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_1b.jpg)

Have you ever thought about writing some text on the display of your VX-8? Or maybe about setting the time to 89:91:75? Now you know how to do it!

Let's try one more example where there will be no numbers in latitude and longitude fields.
```
$GPGGA,999999.000,ello.ABCD,H,itHub.ABCD,G,1,04
```
And the output is:

![Yaesu VX-8DE](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_2.jpg)

Note that last two fields of our so called NMEA sentence mean that there was a valid GPS fix and that it was acquired from 4 satellites. This is exactly what we see on the display: satellites icon with number 4 under it.  This is enough for the transceiver to enable APRS beacon transmission. If you press the internet (TX PO) key these coordinates will be transmitted. Obviously, APRS site will mark such packet as invalid.

When I told you that there is no data validation I really meant no validation at all! You don't even have to add checksum as it will be completely ignored by the radio.

If you want to try it your self you'll need a standard programming cable (original or one from AliExpress) and a terminal program. I used [PuTTY](http://www.putty.org/). Open a new session, choose the correct serial port, set it to 9600 8N1 and you are ready to go. Right mouse click in the terminal window pastes the message from the clipboard. After pasting your message press CTRL+M and then CTRL+J. It will send the CR and LF (ASCII 13 and 10) symbols which mark the end of NMEA sentence.

## Hardware

![Schematic](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_gps_connections.png)

The hardware is quite simple. GY-GPS6MV2 GPS module transmits its data to the Arduino which does necessary processing and sends it to the transceiver. Allthough the communication between the original Yaesu FGPS2 module and the radio is bi-directional, in my project I decided to make it unidirectional from GPS to radio. Not only is the bi-directional communication useless in this case but also requires one more serial port. This configuration uses only one serial port to communicate with both devices.

Important! The serial port on the Arduino board is also used for programming the device. Leaving the GPS connected to the serial RX pin will cause a conflict and make uploading the firmware impossible. That's why there is a switch which disconnects GPS output from serial input.

On the contrary, using USB serial when the transceiver is connected not only is possible but also is recommended for debugging purposes. You can use any terminal program to monitor Arduino's output even when the radio is connected.

Voltage divider formed by 240 Ohm and 470 Ohm resistors converts the logic levels from 5V used by Arduino to 3.3V required by the transceiver. Two two-color LEDs are used for indication. The left one blinks red if a GGA message was rejected and green if it was sent to the radio. The right one does the same for RMC messages.

All the setup is very similar to [David Fannin's GPS project](https://github.com/dfannin/arduino-vx8r-gps). Main difference is that his project implements bi-directional communication using software serial port emulation for the second port.

The GY-GPS6MV2 GPS recevier and the Arduino Nano clone were both purchases on AliExpress for less than $15.

## Software

To be completed...

## Prototypes

### Arduino Nano

![Prototype](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_prototype_1.jpg)

![Prototype](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_prototype_2.jpg)

![Prototype](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_prototype_3.jpg)

### Standalone Microcontroller v1.0

The prototype v1.0 is built on a dedicated PCB with a standalone ATmega328P-20PU microcontroller in 28 pin DIP package.

Refer to [prototype_v1.0.md](https://github.com/4z7dtf/vx8_gps/blob/master/docs/prototype_v1.0.md) for details.

## Field tests

The unit was tested in trains. Videos from the tests are available.

1. [Test 1: Herzliya station](http://www.youtube.com/watch?v=POHEborbWdw)
2. [Test 2: Kiryat Gat station](https://www.youtube.com/watch?v=_s_EKdAwrpI)

![GPS in train](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_test_0.jpg)

![GPS in train](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_test_1.jpg)


## Plans for future develompent

1. [x] Running all the system at 3.3V. Done. See [prototype_v1.0.md](https://github.com/4z7dtf/vx8_gps/blob/master/docs/prototype_v1.0.md) for details.
2. [x] Reducing the microcontroller speed to 8MHz or even lower. Done. See [Release v1.0](https://github.com/4z7dtf/vx8_gps/releases/tag/v1.0) ([commit 9e8273a](https://github.com/4z7dtf/vx8_gps/commit/9e8273abed9b53197791b6e432649a282e6ca909)) .
3. [ ] Adding a rechargeable battery and making the device USB chargeable.
4. [ ] Building the project on a PCB instead of using ready modules.
5. [ ] Assembling the device in one enclosure box.
6. [ ] Making the module compatible with Nikon DSLRs.

Optional/questionable features:

1. [ ] USB serial chip to use the device as a programming cable.
2. [ ] microSD card slot and keeping logs of the tracks.
3. [ ] LCD display for displaying current GPS data.

## Links
1. [Reverse Engineering the Yaesu VX-8DR GPS Interface](http://lingnik.com/2013/02/09/reverse-engineering-yaesu-vx-8dr-gps-interface.html)
2. [FGPS2 data samples (VX-8R Yahoo Group)](https://groups.yahoo.com/neo/groups/VX_8R/conversations/topics/7719)
3. [David Fannin's Yaesu VX-8R Handheld Transmitter-compatible GPS](https://github.com/dfannin/arduino-vx8r-gps)
4. [NMEA to Yaesu VX-8 by G7UHN](http://alloutput.com/amateur-radio/nmea-to-yaesu-vx-8/)
5. [VX-8DR and GPS by VK3YY](https://vk3yy.wordpress.com/2015/07/24/vx-8dr-and-gps/)
6. [GPS - NMEA sentence information](http://aprs.gids.nl/nmea/)
7. [MTK NMEA checksum calculator](http://www.hhhh.org/wiml/proj/nmeaxor.html)
8. [avr-libc 2.0.0: <avr/interrupt.h>: Interrupts](http://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html)

## Questions? Suggestions?
You are more than welcome to contact me with any questions, suggestions or propositions regarding this project. You can:

1. Visit [my QRZ.COM page](https://www.qrz.com/db/4Z7DTF)
2. Visit [my Facebook profile](https://www.facebook.com/Dima.Meln)
3. Write me an email to iosaaris =at= gmail dot com

73 de 4Z7DTF

![73's](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/docs/images/vx8_73.jpg)
