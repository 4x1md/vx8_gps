# DIY GPS module for Yaesu VX-8DR/VX-8DE handheld transceiver
GPS module for Yaesu VX-8DR/DE handheld transceivers with ublox NEO-6M chip and Arduino Nano.

## Overview

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

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_0.jpg)

Allthough the sentence accords with the standard, the transceiver isn't able to process it correctly.

Actually setting number of sybols in each field is what all this project is about! If the second issue didn't exist we could connect the ublox NEO-6M chip based module directly to the transceiver.

### NMEA data validation
Here the things get even more interesting. The Yaesu VX-8 does no validation on the NMEA data. When I say no validation, I mean **no validation at all**! Let's look at the following NMEA sentence.
```
$GPGGA,999999.000,73DE.0000,!,Z7DTF.NOPQ,4,1,04
```
The time in the first field is invalid, the latitude and longitude fields have letters instead of numbers, the North/South and East/West fields have invalid characters as well. The checksum is missing. Should it be thrown away by the receiving device? Yes, sure! But let's see how VX-8 will respond!

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_1a.jpg)

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_1b.jpg)

Have you ever thought about writing some text on the display of your VX-8? Or maybe about setting the time to 89:91:75? Now you know how to do it!

Let's try one more example where there will be no numbers in latitude and longitude fields.
```
$GPGGA,999999.000,ello.ABCD,H,itHub.ABCD,G,1,04
```
And the output is:

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_2.jpg)

Note that last two fields of our so called NMEA sentence mean that there was a valid GPS fix and that it was acquired from 4 satellites. This is exactly what we see on the display: satellites icon with number 4 under it.  This is enough for the transceiver to enable APRS beacon transmission if you press the internet (TX PO) key these coordinates will be transmitted. Obviously, APRS site will mark such packet as invalid.

When I told you that there is no data validation I really meant no validation at all!

If you want to try it your self you'll need a standard programming cable (original or one from AliExpress) and a terminal program. I used [PuTTY](http://www.putty.org/). Open a new session, choose the correct serial port, set it to 9600 8N1 and you are ready to go. Rigth mouse click in the terminal window pastes the message from the clipboard. After pasting the message press CTRL+M and then CTRL+J. It will produce the CR and LF (ASCII 13 and 10) symbols which mark end of NMEA sentence.

## Hardware

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_gps_connections.png)

The hardware is quite simple. GY-GPS6MV2 GPS module transmits its data to the Arduino which does necessary processing and sends it to the transceiver. Allthough the communication between the original Yaesu FGPS2 module and the radio is bi-directional, in my project I decided to make it unidirectional from GPS to radio. Not only is the bi-directional communication useless in this case but also requires one more serial port. This configuration uses only one serial port to communicate with both devices.

Important! The serial port on the Arduino board is also used for programming the device. Leaving the GPS connected to the serial RX pin will cause a conflict and make uploading the firmware impossible. That's why there is a switch which disconnects GPS output from serial input.

On the contrary, using USB serial when the transceiver is connected not only is possible but also is recommended for debugging purposes. You can use any terminal program to monitor Arduino's output even when the radio is connected.

Voltage divider formed by 240 Ohm and 470 Ohm resistors converts the logic levels from 5V used by Arduino to 3.3V required by the transceiver. Two two-color LEDs are used for indication. The left one blinks red if a GGA message was rejected and green if it was sent to the radio. The right one does the same form RMC messages.

All the setup is very similar to [David Fanin's GPS project](https://github.com/dfannin/arduino-vx8r-gps). Main difference is that his project implements bi-directional communications using a software serial port emulation for the second port.

The GY-GPS6MV2 GPS recevier and the Arduino Nano clone were both purchases on AliExpress for less than $15.

## Software

## How my prototype looks like

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_prototype_1.jpg)

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_prototype_2.jpg)

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_prototype_3.jpg)

## Field test

The prototype was tested in a train.  [Click here](http://www.youtube.com/watch?v=POHEborbWdw) to see the video.

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_test_0.jpg)

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_test_1.jpg)


## Plans for the future

## Useful links
1. [Reverse Engineering the Yaesu VX-8DR GPS Interface](http://lingnik.com/2013/02/09/reverse-engineering-yaesu-vx-8dr-gps-interface.html)
2. [FGPS2 data samples (VX-8R Yahoo Group)](https://groups.yahoo.com/neo/groups/VX_8R/conversations/topics/7719)
3. [David Fannin's Yaesu VX-8R Handheld Transmitter-compatible GPS](https://github.com/dfannin/arduino-vx8r-gps)
4. [NMEA to Yaesu VX-8](http://alloutput.com/amateur-radio/nmea-to-yaesu-vx-8/)
5. [GPS - NMEA sentence information](http://aprs.gids.nl/nmea/)
6. [MTK NMEA checksum calculator](http://www.hhhh.org/wiml/proj/nmeaxor.html)

## Questions? Suggestions?
You are more than welcome to contact me with any questions, suggestions or propositions regarding this project. You can:

1. Visit [my QRZ.COM page](https://www.qrz.com/db/4Z7DTF)
2. Visit [my Facebook profile](https://www.facebook.com/Dima.Meln)
3. Write me an email to iosaaris =at= gmail dot com

73 de 4Z7DTF

![BlockDiagram](https://raw.githubusercontent.com/4z7dtf/vx8_gps/master/VX8_GPS/Docs/Images/vx8_73.jpg)
