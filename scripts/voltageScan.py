#!/usr/local/bin/python3.6
# -*- coding: utf-8 -*-


import math
import os
import stat
import sys
import argparse
import subprocess
from subprocess import Popen, PIPE, STDOUT
import shutil

from panel import *
import runAcquisition 

def main(argv):

    voltage_list = ['54','54.2','54.4','54.6','54.8','55','55.2','55.4','55.6','55.8','56','56.2','56.4','56.6','56.8','57','57.2','57.4','57.6','57.8','58','58.2','58.4','58.6']
    slice_time = '3600'

    
    #thisdict = {
      #'config' : 'file.txt',
      #'time'   : '1000',
      #'folder' : 'path',
      #'keep'   : '0',
      #}
    #dummy.main(thisdict)
    #dummy.main('dummy --config file --time t --file f --keep k')

    component = Component(initial_Voltage)

    print('Initializing Power supply...')
    component.begin_com()
    component.switch_off_hv() # always start with off

    wait_for_rump_down(component,10,500)
    component.send_value(0,"10")   # set voltage point on chip to 10V
    component.switch_on_hv()  # enable hv

    # acquire
    for v in voltage_list:
        print('Setting acq at voltage ', v)
        scan_step(component,v,slice_time)

    #exit but first switch off HV
    component.switch_off_hv()
    wait_for_rump_down(component,10,500)
    component.send_value(0,"10")   # set voltage point on chip to 10V, so next time is not switch on at high voltage by mistake...
    time.sleep(0.1)
    print('Done, goodbye!')


def scan_step(component,v_set,slice_time):
    time.sleep(0.10)
    component.send_value(0,v_set)   # set voltage
    component.switch_on_hv()  # enable hv
    #monitor and wait for ramp up of voltage
    wait_for_rump_up(component,v_set,100)
    string_out = 'Acquiring for '+ slice_time + ' seconds at ' + v_set + 'V...'
    print(string_out)
    
    thisdict = {
      'config' : 'config_two_febs.txt',
      'time'   : slice_time,
      'folder' : v_set,
      'keep'   : '0',
      'single' : '1'
      }
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
   main(sys.argv[1:])
