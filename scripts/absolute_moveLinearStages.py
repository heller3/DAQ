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

from panel import Component
import zaber_library as zls


def main(args):


    useLinearStages=False

    if args['xmicrons'] is None:
      if args['zmicrons'] is None:
        print('No position given in both x and z. Nothing to do. Quitting...')
        sys.exit(0)

    useLinearStages=True
    x_distance = 0
    z_distance = 0

    if args['xmicrons'] is None:
      x_distance = -1
    else:
      x_distance = float(args['xmicrons'])
    if args['zmicrons'] is None:
      z_distance = -1
    else:
      z_distance = float(args['zmicrons'])

    if x_distance == -1:
      if z_distance == -1:
        print('Position given is negative or invalide for both x and z. Nothing to do. Quitting...')
        sys.exit(0)

    #
    stages = 0



    #configure linear stages
    if useLinearStages:
      try:
        stages = serial.Serial("/dev/LStages", 9600, 8, 'N', 1, timeout=5)
      except SerialException:
        print("Error opening com port. Quitting.")
        sys.exit(0)
      print("Opening " + stages.portstr)

      ## Print current position
      print ('')
      zls.print_current_positions(stages)

      if(x_distance != -1):
        print ('Moving horizontal (x) stage to position [microns] ', x_distance)
        zls.move_stage_absolute(stages,2,x_distance)
        print ('')
        time.sleep(1)

      if(z_distance != -1):
        print ('Moving vertical (z) stage to position [microns] ', z_distance)
        zls.move_stage_absolute(stages,1,z_distance)
        print ('')
        time.sleep(1)
      
      zls.print_current_positions(stages)
      

    print("Done")




if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Python script to move linear stages')
  parser.add_argument('-x','--xmicrons' , help='Move x to N microns',required=False)
  parser.add_argument('-z','--zmicrons' , help='Move z to N microns',required=False)
  args = parser.parse_args()

  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
  #main(sys.argv[1:])
