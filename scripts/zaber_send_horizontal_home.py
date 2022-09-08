#!/usr/bin/python3
# -*- coding: utf-8 -*-


import math
import os
import stat
import sys
import argparse
import subprocess
from subprocess import Popen, PIPE, STDOUT
import shutil
import configparser
import time
import serial
import struct
from serial import SerialException

import minimalmodbus





def send(device, command, data=0):
   # send a packet using the specified device number, command number, and data
   # The data argument is optional and defaults to zero
	packet = struct.pack('<BBl', device, command, data)
	stages.write(packet)


def receive():
   # return 6 bytes from the receive buffer
   # there must be 6 bytes to receive (no error checking)
    r = [0,0,0,0,0,0]
    for i in range (6):
        r[i] = ord(stages.read(1))
    return r

stages = 0


try:
    stages = serial.Serial("/dev/LStages", 9600, 8, 'N', 1, timeout=5)
except SerialException:
    print("Error opening com port. Quitting.")
    sys.exit(0)

print("Opening " + stages.portstr)

#command = 0
#device  = 1
#data    = 0
#send(device,command,data)
#device  = 2
#send(device,command,data)

#command     = 21 #move absolute
#Store Current Position 16
#Return Stored Position 17
#Return Current Position 60

device=2
command=1
data=0

send(device,command,data)
print(receive())
