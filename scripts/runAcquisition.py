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




def main(args):
   
   #parsing args
   
   if args['config'] is None:
     print('No config file provided')
     sys.exit(1)
   else:
     config = args['config']
  
   if args['time'] is None:
     print('No time provided')
     sys.exit(1)
   else:
     time = args['time']
  
   if args['folder'] is None:
     print('No folder provided')
     sys.exit(1)
   else:
     folder = args['folder']
  
   if args['keep'] is None:
     print('No keep flag provided')
     sys.exit(1)
   else:
     keep = args['keep']
   
   singleRun = '0'
   
   if args['single'] is None:
     print('Setting default = non single acq')
   else:
     singleRun = args['single']
   #parser = argparse.ArgumentParser(description='Python script to start acq')
   #parser.add_argument('-c','--config' , help='Config file',required=True)
   #parser.add_argument('-t','--time'   , help='Time per acq',required=True)
   #parser.add_argument('-f','--folder' , help='Folder name',required=True)
   #parser.add_argument('-k','--keep'   , help='0 = discard raw data; 1 = keep raw data',required=True)
   #args = parser.parse_args()

   #print values
   print ("Config file      = %s" % config )
   print ("Time per acq [s] = %s" % time )
   print ("Folder name      = %s" % folder )
   if keep == '0':
     print ("Raw data will be discarded")
   else:
     print ("Keeping row data")
   if singleRun == '0':
     print ("Multiple slices, will stop with q keyboard hit")
   else: 
     if singleRun == '1':
       print ("Single slice, will stop after time finished")
       #print ("singleRun initial = ", singleRun)
     else:
       print ("-s/--single value not valid = ", singleRun)
       print ("Aborting...")
       sys.exit(1)


   counter = 0

   #make RootTTrees folder
   finalFolder = "Run_" + folder + "/RootTTrees"
   os.makedirs(finalFolder)

   #copy config file into main acq folder
   copyConfiFile = "Run_" + folder + "/" + config
   shutil.copyfile(config, copyConfiFile)


   run = 1
   t_folder = folder + "/" + str(counter)
   cmd = ['readout','-c', config,'-t', time, '-f', t_folder,'--start']
   returncode = subprocess.Popen(cmd).wait()
     #print(child.returncode)
   if returncode == 255:
     run = 0

   fName = "convert.sh"
   
   if singleRun == '1': #break immediately if it's just one run, do not wait for 'q' from keyboard
       run = 0

   while run == 1 :
     #convert old data
     eventsFile = 'Run_' + t_folder + '/events.dat'
     #read start time before converting data 
     fileStartTime = open("startTime.txt", "r") 
     startTime = fileStartTime.readline()
     fileStartTime.close()
     
     #create script

     f = open(fName,'w')
     f.write("#!/bin/bash\n")
     f.write("events -o %s " %eventsFile)
     f.write("--input0 Run_%s/binary740.dat "   %t_folder)
     f.write("--input1 Run_%s/binary742_0.dat " %t_folder)
     f.write("--input2 Run_%s/binary742_1.dat " %t_folder)
     f.write(" && convertToRoot ")
     f.write("--input %s "   %eventsFile)
     f.write("--output-folder Run_%s " %folder)
     f.write("--frame %d " %counter)
     f.write("--time %s " %startTime)
     if keep == '0':
       f.write(" && rm Run_%s/binary740.dat " %t_folder )
       f.write(" && rm Run_%s/binary742_0.dat " %t_folder )
       f.write(" && rm Run_%s/binary742_1.dat " %t_folder )
       f.write(" && rm %s " %eventsFile )
       f.write(" && rm -r Run_%s " %t_folder )
     f.close()
     #and make it executable
     st = os.stat(fName)
     os.chmod(fName, st.st_mode | stat.S_IEXEC)

     convertCmd = ['./' + fName]
     convertCode = subprocess.Popen(convertCmd,shell=True).poll()
     print(convertCode)

     #start the next acq
     counter = counter + 1
     t_folder = folder + "/" + str(counter)
     cmd = ['readout','-c', config,'-t', time, '-f', t_folder,'--start']
     returncode = subprocess.Popen(cmd).wait()

     #print(child.returncode)
     #print ("singleRun in loop = ", singleRun)
     if returncode == 255:
       run = 0
     

   #convert last data
   #convert old data
   eventsFile = 'Run_' + t_folder + '/events.dat'
   fileStartTime = open("startTime.txt", "r") 
   startTime = fileStartTime.readline()
   fileStartTime.close()
   #create script
   f = open(fName,'w')
   f.write("#!/bin/bash\n")
   f.write("events -o %s " %eventsFile)
   f.write("--input0 Run_%s/binary740.dat "   %t_folder)
   f.write("--input1 Run_%s/binary742_0.dat " %t_folder)
   f.write("--input2 Run_%s/binary742_1.dat " %t_folder)
   f.write(" && convertToRoot ")
   f.write("--input %s "   %eventsFile)
   f.write("--output-folder Run_%s " %folder)
   f.write("--frame %d " %counter)
   f.write("--time %s " %startTime)
   if keep == '0':
     f.write(" && rm Run_%s/binary740.dat " %t_folder )
     f.write(" && rm Run_%s/binary742_0.dat " %t_folder )
     f.write(" && rm Run_%s/binary742_1.dat " %t_folder )
     f.write(" && rm %s " %eventsFile )
     f.write(" && rm -r Run_%s " %t_folder )
   f.close()
   #and make it executable
   st = os.stat(fName)
   os.chmod(fName, st.st_mode | stat.S_IEXEC)

   convertCmd = ['./' + fName]
   print("Converting latest frame...!\n")
   convertCode = subprocess.Popen(convertCmd,shell=True).wait()
   os.remove(fName)

   #move files in the same folder
   print("Moving files...!\n")
   #
   print("Done!\n")


if __name__ == "__main__":
  #parsing args
  parser = argparse.ArgumentParser(description='Python script to start acq')
  parser.add_argument('-c','--config' , help='Config file',required=True)
  parser.add_argument('-t','--time'   , help='Time per acq',required=True)
  parser.add_argument('-f','--folder' , help='Folder name',required=True)
  parser.add_argument('-k','--keep'   , help='0 = discard raw data; 1 = keep raw data',required=True)
  parser.add_argument('-s','--single' , help='0 = not single slice; 1 = single slice',required=False) #default 0
  args = parser.parse_args()
  
  # Convert the argparse.Namespace to a dictionary: vars(args)
  main(vars(args))
  sys.exit(0)
