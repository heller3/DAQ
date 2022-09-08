#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import serial
from serial import SerialException
import zaber_library as zls


stages = 0

try:
    stages = serial.Serial("/dev/LStages", 9600, 8, 'N', 1, timeout=5)
except SerialException:
    print("Error opening com port. Quitting.")
    sys.exit(0)

print("Opening " + stages.portstr)
print('')
zls.print_maximum_positions(stages)