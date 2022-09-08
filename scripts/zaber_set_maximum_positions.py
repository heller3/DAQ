#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import serial
from serial import SerialException
import zaber_library as zls
import argparse

def main(args):
    
    if args['xmicrons'] is None:
        if args['zmicrons'] is None:
            print('No x or z max positions given. Nothing to do. Quitting...')
            sys.exit(0)
    
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
        print('Position given is not valid or absent for both x and z. Nothing to do. Quitting...')
        sys.exit(0)
    
    stages = 0
    
    try:
        stages = serial.Serial("/dev/LStages", 9600, 8, 'N', 1, timeout=5)
    except SerialException:
        print("Error opening com port. Quitting.")
        sys.exit(0)
    
    print("Opening " + stages.portstr)
    print('')
    
    if(x_position != -1):
        print ('Setting horizontal (x) stage limit position [microns] = ', x_position)
        zls.set_maximum_position(stages,2,x_position)
        print ('')
    if(z_position != -1):
        print ('Setting vertical (z) stage limit position [microns]   = ', z_position)
        zls.set_maximum_position(stages,1,z_position)
        print ('')
    
    #device = 2 # aka x  
    print('')
    #zls.set_maximum_position(stages,device,position)
    # and confirm
    print('')
    zls.print_maximum_positions(stages)



if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Python script to set maximum of linear stage')
  parser.add_argument('-x','--xmicrons' , help='Set x maximum to N microns',required=False)
  parser.add_argument('-z','--zmicrons' , help='Set z maximum to N microns',required=False)
  args = parser.parse_args()

  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
