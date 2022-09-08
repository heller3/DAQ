#!/usr/bin/python3
# -*- coding: utf-8 -*-


import sys
import serial
from serial import SerialException
import argparse
import time


import zaber_library as zls

def main(args):

    useLinearStages=False
    if args['xmicrons'] is None:
      if args['zmicrons'] is None:
        print('No position given in both x and z. Nothing to do. Quitting...')
        sys.exit(0)
    useLinearStages=True
    x_position = -1
    z_position = -1

    if args['xmicrons'] is None:
      x_position = -1
    else:
      x_position = float(args['xmicrons'])
    if args['zmicrons'] is None:
      z_position = -1
    else:
      z_position = float(args['zmicrons'])

    if x_position == -1:
      if z_position == -1:
        print('No Position given for both x and z. Nothing to do. Quitting...')
        sys.exit(0)

    #
    stages = 0


    try:
        stages = serial.Serial("/dev/LStages", 9600, 8, 'N', 1, timeout=5)
    except SerialException:
        print("Error opening com port. Quitting.")
        sys.exit(0)
    
    print("Opening " + stages.portstr)
    print('')
    zls.print_current_positions(stages)
    
    #print ('Moving horizontal (x) stage to position [microns] ', x_position)
    #zls.move_stage_absolute(stages,2,x_position)
    if(x_position != -1):
        print ('Moving horizontal (x) stage to position [microns] ', x_position)
        zls.move_stage_absolute(stages,2,x_position)
        time.sleep(1)

    if(z_position != -1):
        print ('Moving vertical (z) stage to position [microns] ', z_position)
        zls.move_stage_absolute(stages,1,z_position)
        time.sleep(1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Python script to move linear stages')
  parser.add_argument('-x','--xmicrons' , help='Move x to position [microns]',required=False)
  parser.add_argument('-z','--zmicrons' , help='Move z to position [microns]',required=False)
  args = parser.parse_args()

  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
  #main(sys.argv[1:])