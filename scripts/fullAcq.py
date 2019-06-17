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

import minimalmodbus

from panel import *
import runAcquisition 


def set_jog_step(motor1,motor2):
    #motor2.write_register(1025,375,functioncode=16)
    motor1.write_register(4169,375,functioncode=16)
    motor2.write_register(4169,375,functioncode=16)


def jog_both_arms(motor1,motor2):
    time.sleep(0.1)

    #set to STOP both arms, just to be sure
    motor1.write_register(125,32,functioncode=16)
    motor2.write_register(125,32,functioncode=16)
    time.sleep(0.1)
   
    #set to + and - JOG both arms
    motor1.write_register(125,4096,functioncode=16)
    motor2.write_register(125,8192,functioncode=16)
    
    #wait for both arms to stop
    time.sleep(0.5)
    # start acquisition
    #time.sleep(0.2)
    #set to STOP both arms
    motor1.write_register(125,32,functioncode=16)
    motor2.write_register(125,32,functioncode=16)
    #time.sleep(0.2)
    

def stop_both_arms(motor1,motor2):
    motor1.write_register(125,32,functioncode=16)
    motor2.write_register(125,32,functioncode=16)


def main(args):
  
    MAX_ACQS = 1000 # maximum acqs in config file
    if args['config'] is None:
      print('No config file provided')
      sys.exit(1)
      
    useMotors = False
    
    # read config file
    # check if it is there
    cfgFile = args['config']
    if os.path.isfile(cfgFile) is False:
      print('Config file not found')
      sys.exit(1)
    else:
      print('Using config file',cfgFile)
    config = configparser.ConfigParser()
    config.read(cfgFile)
    #print(config.sections())
    
    #check if config file has GENERAL section
    if config.has_section('GENERAL') == False:
      print('ERROR! No general section in config file!')
      sys.exit(1)
    if config.has_option('GENERAL', 'name') == False:
      print('ERROR! No acq name provided! ')
      sys.exit(1)
    
    #check if config file sections all have the proper keys 
    for a in range(0,MAX_ACQS):
      section_name = 'ACQ_' + str(a)
      if config.has_section(section_name):
        # check if all keys are there
        if config.has_option(section_name, 'config') == False:
          print('ERROR! Missing key config for section ' , section_name)
          sys.exit(1)
        if config.has_option(section_name, 'time') == False:
          print('ERROR! Missing key time for section '   , section_name)
          sys.exit(1)
        if config.has_option(section_name, 'V_FEB_0') == False:
          print('ERROR! Missing key V_FEB_0 for section ', section_name)
          sys.exit(1)
        if config.has_option(section_name, 'V_FEB_1') == False:
          print('ERROR! Missing key V_FEB_1 for section ', section_name)
          sys.exit(1)
    
    
    ## INITIALIZE POWER SUPPLIES
    # start the voltage 
    component = Component(initial_Voltage)
    print('Initializing Power supply...')    
    component.begin_com()
    component.switch_off_hv() # always start with off
    wait_for_rump_down(component,10,500)
    component.send_value(0,"10")   # set voltage point on chip to 10V
    component.switch_on_hv()  # enable hv
    
    # configure motors
    if useMotors:
      motor1 = minimalmodbus.Instrument('/dev/ttyUSB1', 1) # port name, slave address (in decimal)
      motor2 = minimalmodbus.Instrument('/dev/ttyUSB1', 2) # port name, slave address (in decimal)
      motor1.serial.baudrate = 9600
      motor1.serial.bytesize = 8
      motor1.serial.parity   = serial.PARITY_EVEN
      motor1.serial.stopbits = 1
      motor1.serial.timeout  = 0.1
      motor1.mode = minimalmodbus.MODE_RTU
      set_jog_step(motor1,motor2)
    
    ## MAIN LOOP on acqs
    for a in range(0,MAX_ACQS):
      section_name = 'ACQ_' + str(a)
      if config.has_section(section_name):
        
        # SET VOLTAGES FOR THIS ACQ
        if useMotors:
          print ('-----> ANGLE ', a)
        print('Setting acq at voltage ', config[section_name]['V_FEB_0'])
        time.sleep(0.10)
        #for now only set FEB 0 (FEB 1 is still on keythley)
        component.send_value(0,config[section_name]['V_FEB_0'])   # set voltage
        component.switch_on_hv()  # enable hv
        # monitor and wait for ramp up of voltage
        wait_for_rump_up(component,config[section_name]['V_FEB_0'],100)
        
        # START DAQ
        # create the dictionary
        folder_name = config['GENERAL']['name'] + '_V0_' + config[section_name]['V_FEB_0'] + '_V1_' + config[section_name]['V_FEB_1'] + "_t_" + config[section_name]['time'] 
        thisdict = {
          'config' : config[section_name]['config'],
          'time'   : config[section_name]['time'],
          'folder' : folder_name,
          'keep'   : '0',
          'single' : '1'
        }
        # run and wait...
        scan_step(config[section_name]['time'],config[section_name]['V_FEB_0'],config[section_name]['V_FEB_1'],thisdict)
        
        # rotate arms
        if useMotors:
          jog_both_arms(motor1,motor2)
          time.sleep(1)
        
    # stop arms
    if useMotors:
      stop_both_arms(motor1,motor2)
    
    # END: exit but first switch off HV
    component.switch_off_hv()
    wait_for_rump_down(component,10,500)
    component.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
    time.sleep(0.1)
    print('Done, goodbye!')


def scan_step(t,v0,v1,thisdict):
    
    string_out = 'Acquiring for ' + t + ' seconds at V_FEB_0 = ' + v0 + 'V and  V_FEB_0 = ' + v1  
    print(string_out)
    runAcquisition.main(thisdict) #start acq
    time.sleep(0.1)


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
  parser = argparse.ArgumentParser(description='Python script to start acq')
  parser.add_argument('-c','--config' , help='Config file',required=True)  
  args = parser.parse_args()
  
  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
  #main(sys.argv[1:])
