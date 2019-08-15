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
from serial import SerialException

import minimalmodbus

from panel import Component
import runAcquisition


def set_jog_step(motor,value):
    #motor2.write_register(1025,375,functioncode=16)
    steps_to_angle = 0.02
    steps = int(abs(value) / steps_to_angle)
    #print(steps)
    motor.write_register(4169,steps,functioncode=16)
    #motor2.write_register(4169,value,functioncode=16)


def jog_arm(motor,value):

    angle_for_half_second = 7.5
    sleep_time = abs(value / 7.5)
    #print(sleep_time)
    time.sleep(0.1)

    #set to STOP, just to be sure
    motor.write_register(125,32,functioncode=16)
    #motor2.write_register(125,32,functioncode=16)
    time.sleep(0.1)

    #set to + or - JOG
    if(value > 0):
      motor.write_register(125,4096,functioncode=16)
    if(value < 0):
      motor.write_register(125,8192,functioncode=16)

    #wait for arm to stop
    time.sleep(sleep_time)
    # start acquisition
    #time.sleep(0.2)
    #set to STOP arm
    motor.write_register(125,32,functioncode=16)
    #motor2.write_register(125,32,functioncode=16)
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
    initial_Voltage=19.001
    repetitions = 1
    acquire_data = True
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
    if config.has_option('GENERAL', 'use_v1') == False:
      print('ERROR! You need to specify if voltage supply 1 is used or not, with use_v1 ! ')
      sys.exit(1)
    if config.has_option('GENERAL', 'use_v2') == False:
      print('ERROR! You need to specify if voltage supply 1 is used or not, with use_v2 ! ')
      sys.exit(1)
    if config.has_option('GENERAL', 'use_motors') == False:
      print('ERROR! You need to specify if motors are used or not, with use_motors ! ')
      sys.exit(1)


    if config.has_option('GENERAL', 'repetitions') == True:
        repetitions = int(config['GENERAL']['repetitions'])
    if config.has_option('GENERAL', 'acquire_data') == True:
        if(config['GENERAL']['acquire_data'] == 'false'):
          acquire_data = False
    if(config['GENERAL']['use_v1'] == 'true'):
      useV1 = True
    else:
      useV1 = False

    if(config['GENERAL']['use_v2'] == 'true'):
      useV2 = True
    else:
      useV2 = False

    if(config['GENERAL']['use_motors'] == 'true'):
      useMotors = True
    else:
      useMotors = False


    #if(useMotors):
        #print("aaaaaaaaaaaaaaaaaaaaaaa")

    #check if all config file sections have the proper keys
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
        if config.has_option(section_name, 'V_FEB_1') == False:
          print('ERROR! Missing key V_FEB_1 for section ', section_name)
          sys.exit(1)
        if config.has_option(section_name, 'V_FEB_2') == False:
          print('ERROR! Missing key V_FEB_2 for section ', section_name)
          sys.exit(1)


    ## INITIALIZE POWER SUPPLIES
    # start the voltage supplies
    component1 = 0
    component2 = 0
    #motor_steps = float(config['GENERAL']['motor_steps'])
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

    #try:

    #except:
    #  print('ERROR! Power supply for FEB1 not found!')
    #  exit_with_grace(0,0)
    #try:

    #except:
    #  print('ERROR! Power supply for FEB2 not found!')
    #  exit_with_grace(component1,0)
    #print('Initializing Power supplies...')
    #exit_with_grace(component1,component2)
    #exit()


    #exit()

    # configure motors
    if useMotors:
      motor1 = minimalmodbus.Instrument('/dev/Motors', 1) # port name, slave address (in decimal)
      motor2 = minimalmodbus.Instrument('/dev/Motors', 2) # port name, slave address (in decimal)
      motor1.serial.baudrate = 9600
      motor1.serial.bytesize = 8
      motor1.serial.parity   = serial.PARITY_EVEN
      motor1.serial.stopbits = 1
      motor1.serial.timeout  = 0.1
      motor1.mode = minimalmodbus.MODE_RTU
      #set_jog_step(motor1,motor2,motor_steps)

    ## MAIN LOOP on acqs
    for r in range(0,repetitions):
        print ('-----> REPETITION ', r)
        for a in range(0,MAX_ACQS):
          section_name = 'ACQ_' + str(a)
          if config.has_section(section_name):
            angle1 = 0.
            angle2 = 0.

            if useMotors:

              # rotate arms
              angle1 = 0.
              angle2 = 0.

              # arm 1
              if config.has_option(section_name, 'angle1') == False:
                print('ERROR! No rotation angle provided for arm 1 !')
                exit_with_grace(component1,component2)
              else:
                angle1 = float(config[section_name]['angle1'])

              # arm 2
              if config.has_option(section_name, 'angle2') == False:
                print('ERROR! No rotation angle provided for arm 2 !')
                exit_with_grace(component1,component2)
              else:
                # invert angle to be in the same reference of arm 1
                angle2 = -float(config[section_name]['angle2'])

              if(angle1 != 0):
                print ('Rotating ARM 1 of degrees ', angle1)
                set_jog_step(motor1,angle1) #fix me, set to angle
                jog_arm(motor1,angle1)
                time.sleep(1)

              if(angle2 != 0):
                print ('Rotating ARM 2 of degrees ', -angle2)
                set_jog_step(motor2,angle2) #fix me, set to angle
                jog_arm(motor2,angle2)
                time.sleep(1)

            # SET VOLTAGES FOR THIS ACQ
            if useV1:
              print('Setting FEB1 at voltage ', config[section_name]['V_FEB_1'])
              time.sleep(0.10)
              #set FEB 1
              component1.send_value(0,config[section_name]['V_FEB_1'])   # set voltage
              component1.switch_on_hv()  # enable hv
              # monitor and wait for ramp up of voltage
              wait_for_rump_up(component1,config[section_name]['V_FEB_1'],100)
            if useV2:
              #set FEB 2
              print('Setting FEB2 at voltage ', config[section_name]['V_FEB_2'])
              time.sleep(0.10)
              component2.send_value(0,config[section_name]['V_FEB_2'])   # set voltage
              component2.switch_on_hv()  # enable hv
              # monitor and wait for ramp up of voltage
              wait_for_rump_up(component2,config[section_name]['V_FEB_2'],100)


            if(acquire_data):
              # START DAQ
              # create the dictionary
              folder_name = config['GENERAL']['name'] + '_' + section_name + '_Angle1_' + str(angle1) + '_Angle2_' + str(angle2) + '_V1_' + config[section_name]['V_FEB_1'] + '_V2_' + config[section_name]['V_FEB_2'] + "_t_" + config[section_name]['time']
              thisdict = {
                'config' : config[section_name]['config'],
                'time'   : config[section_name]['time'],
                'folder' : folder_name,
                'keep'   : '0',
                'single' : '1'
              }
              # run and wait...
              scan_step(config[section_name]['time'],config[section_name]['V_FEB_1'],config[section_name]['V_FEB_2'],thisdict)




    # stop arms
    if useMotors:
      stop_both_arms(motor1,motor2)

    # END: exit but first switch off HV
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

def scan_step(t,v1,v2,thisdict):

    string_out = 'Acquiring for ' + t + ' seconds at V_FEB_1 = ' + v1 + 'V and  V_FEB_2 = ' + v2
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
