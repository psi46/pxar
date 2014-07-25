#!/bin/bash

# The Keithley should be set to Baud rate of 57600, 8 bits, None parity, 
# <CR+LF> terminator, None flow control.

# you likely have to modify ttyUSB0 according to where it is really mounted on your system

chmod a+r /dev/ttyUSB0
chmod a+w /dev/ttyUSB0
chmod a+x /dev/ttyUSB0

stty -F /dev/ttyUSB0 57600 cs8 clocal cread cr3 echo iexten hupcl

stty -a -F /dev/ttyUSB0

