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





def main(args):
    
    useV1 = True
    useV2 = True
    
    if args['v1'] is None:
      if args['v2'] is None:
        print('No voltage value given for both power supplies. Nothing to do. Quitting...')
        sys.exit(0)
        
    v1 = 0
    v2 = 0
    if args['v1'] is None:
      useV1 = False
    else:
      v1 = args['v1']
    if args['v2'] is None:
      useV2 = False
    else:
      v2 = args['v2']
    
    
    
    initial_Voltage=19.001
    
    component1 = 0
    component2 = 0

    if(useV1):
      # first power supply
      # v supply feb 1
      try:
        ser1 = serial.Serial(
          port='/dev/vSupply1',
          baudrate = 115200,
          parity=serial.PARITY_NONE,
          stopbits=serial.STOPBITS_ONE,
          bytesize=serial.EIGHTBITS,
          timeout=1
          )
      except: #FIXME doesn't really work. why?
        print('ERROR! Power supply for FEB1 not found!')
        exit_with_grace(0,0)

      component1 = Component(initial_Voltage,ser1)
      component1.begin_com()
      component1.set_rump_speed()
      component1.switch_off_hv() # always start with off
      wait_for_rump_down(component1,10,500)
      component1.reset()
      #component1.send_value(0,"10")   # set voltage point on chip to 10V
      component1.switch_on_hv()  # enable hv
      #component1.send_value(0,"10")   # set voltage point on chip to 10V
    if(useV2):
      # second power supply
      # v supply feb 2
      try:
        ser2 = serial.Serial(
          port='/dev/vSupply2',
          baudrate = 115200,
          parity=serial.PARITY_NONE,
          stopbits=serial.STOPBITS_ONE,
          bytesize=serial.EIGHTBITS,
          timeout=1
          )
      except SerialException:
        print('ERROR! Power supply for FEB2 not found!')
        exit_with_grace(component1,0)

      component2 = Component(initial_Voltage,ser2)
      component2.begin_com()
      component2.set_rump_speed()
      component2.switch_off_hv() # always start with off
      wait_for_rump_down(component2,10,500)
      component2.reset()
      #component2.send_value(0,"10")   # set voltage point on chip to 10V
      component2.switch_on_hv()  # enable hv
      #component2.send_value(0,"10")   # set voltage point on chip to 10V
    
    # SET VOLTAGES FOR THIS ACQ
    if useV1:
      print('Setting FEB1 at voltage ', str(v1))
      time.sleep(0.10)
      #set FEB 1
      component1.send_value(0,v1)   # set voltage
      component1.switch_on_hv()  # enable hv
      # monitor and wait for ramp up of voltage
      wait_for_rump_up(component1,v1,100)
    if useV2:
      #set FEB 2
      print('Setting FEB2 at voltage ', str(v2))
      time.sleep(0.10)
      component2.send_value(0,v2)   # set voltage
      component2.switch_on_hv()  # enable hv
      # monitor and wait for ramp up of voltage
      wait_for_rump_up(component2,v2,100)
   

    input("Press ENTER to switch off power supplies")
    exit_with_grace(component1,component2)



def exit_with_grace(component1,component2):
    if(component1 != 0):
      #component1.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
      component1.switch_off_hv()
      wait_for_rump_down(component1,10,500)
      component1.reset()
      #component1.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
      time.sleep(0.1)
    if(component2 != 0):
      #component2.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
      component2.switch_off_hv()
      wait_for_rump_down(component2,10,500)
      component2.reset()
      #component2.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
      time.sleep(0.1)
      print('Done, goodbye!')
      sys.exit(0)



def wait_for_rump_up(component,target,cutOff):
    v_target = float(target)
    loops = 0
    while True:
        loops = loops + 1
        voltage = component.read_voltage()
        print ("V [Volt] = ", voltage, end="\r")
        v_now = float(voltage)
        if (v_target - 0.1) < v_now < (v_target + 0.1):
            break
        if loops > cutOff:
            break
    print('')

def wait_for_rump_down(component,target,cutOff):
    v_target = float(target)
    loops = 0
    while True:
        loops = loops + 1
        voltage = component.read_voltage()
        print ("V [Volt] = ", voltage, end="\r")
        v_now = float(voltage)
        if (v_now < (v_target + 0.1)):
            break
        if loops > cutOff:
            break
    print('')

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Python script to bias boards')
  parser.add_argument('-a','--v1' , help='Voltage 1',required=False)
  parser.add_argument('-b','--v2' , help='Voltage 2',required=False)
  args = parser.parse_args()

  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
  #main(sys.argv[1:])
