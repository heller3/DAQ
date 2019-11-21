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
import runAcquisition





def move_stage(stages,device,distance):
    time.sleep(1)
    command     = 21         #move absolute
    step_size   = 0.49609375 #micrometers
    #z_step      = 500.0 # micrometers
    #x_step      = 3200.0 # micrometers
    N_steps   = int(distance/step_size)
    # print(N_steps)
    # send a packet using the specified device number, command number, and data
    # The data argument is optional and defaults to zero
    packet = struct.pack('<BBl', device, command, N_steps)
    stages.write(packet)
    
    #print(N_steps)
    
    ret = stages.readline()
    if ret[1] == 255:
      print('ERROR! Stage returned error code 255. No movement.')
    else: 
      print('Success')
    #print(type(ret))
    #print(ret)
    #print(ret[0])
    #print(ret[1])
    #print(ret[2])
    #print(ret[3])
    #print(ret[4])
    #print(ret[5])
    #time.sleep(0.10)
    #print(type(ret))
    #asc = ret.readline()
    #print(asc)

#def return_stored_position(stages,device,position):
    ##time.sleep(1)
    #command     = 17         #move absolute
    #packet = struct.pack('<BBl', device, command, position)
    #stages.write(packet)
    #time.sleep(0.10)
    ##ret = stages.readline().decode('ascii')
    #ret = stages.readline()
    #print(type(ret))
    #print(ret)
    #print(ret[0])
    #print(ret[1])
    #print(ret[2])
    #print(ret[3])
    #print(ret[4])
    #print(ret[5])
    ##print(int.from_bytes(ret, byteorder='little'))

#def store_current_position(stages,device,position):
    #command     = 16       
    #packet = struct.pack('<BBl', device, command, position)
    #stages.write(packet)
    #time.sleep(0.10)
    #ret = stages.readline()
    #print(type(ret))
    #print(ret)
    #print(ret[0])
    #print(ret[1])
    #print(ret[2])
    #print(ret[3])
    #print(ret[4])
    #print(ret[5])
    
    
def main(args):


    useLinearStages=False
    

    
    if args['xmicrons'] is None:
      if args['zmicrons'] is None:
        print('No distance given in both x and z. Nothing to do. Quitting...')
        sys.exit(0)
    
    useLinearStages=True
    x_distance = 0
    z_distance = 0
    
    if args['xmicrons'] is None:
      x_distance = 0
    else:
      x_distance = float(args['xmicrons'])
    if args['zmicrons'] is None:
      z_distance = 0
    else:
      z_distance = float(args['zmicrons'])
      
    if x_distance == 0:
      if z_distance == 0:
        print('Distance given is 0 for both x and z. Nothing to do. Quitting...')
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
      
      
      #return_stored_position
      #for i in range(0,16):
      #print ('Stored position in 1')
      #return_stored_position(stages,1,1)
      #print ('Return of store command')
      #store_current_position(stages,1,1)
      #print ('Stored position in 1')
      #return_stored_position(stages,1,1)
      
      
      
      if(x_distance != 0):
        print ('Moving horizontal stage of distance [microns] ', x_distance)
        move_stage(stages,2,x_distance)
        time.sleep(1)

      if(z_distance != 0):
        print ('Moving vertical stage of distance [microns] ', z_distance)
        move_stage(stages,1,z_distance)
        time.sleep(1)

    print("Done")
    



if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Python script to move linear stages')
  parser.add_argument('-x','--xmicrons' , help='Move x by N microns',required=False)
  parser.add_argument('-z','--zmicrons' , help='Move z by N microns',required=False)
  args = parser.parse_args()

  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
  #main(sys.argv[1:])
