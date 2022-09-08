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

zls.print_current_positions(stages)
print("\nStoring current position... \n")
zls.store_positions(stages)
#device=1
#command=16
#data=0
#send(device,command,data)
#device=2
#send(device,command,data)
