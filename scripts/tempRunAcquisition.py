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
import multiprocessing
from multiprocessing import Queue

returncode = 0


### function to convert binary to root data
def convert(eventsFile,folder,output_folder,counter,startTime,keep):
  #prepare command for events program
  cmd = ['events',
         '-o',eventsFile,
         '--input0','Run_'+ folder + '/binary740.dat',
         '--input1','Run_'+ folder + '/binary742_0.dat',
         '--input2','Run_'+ folder + '/binary742_1.dat']
  print ("Generating event file...\n")
  print (cmd)
  #logName = 'log_' + prefix_name + element + '.log'
  #log = open(logName, 'w')
  subprocess.Popen(cmd).wait() #start subprocess and wait for it to finish
  
  #prepare command for convert program
  cmd = ['convertToRoot',
         '--input',eventsFile,
         '--output-folder','Run_'+ output_folder,
         '--frame',str(counter),
         '--time',startTime]
  print ("Converting to root files...\n")
  print (cmd)
  subprocess.Popen(cmd).wait() #start subprocess and wait for it to finish
  
  #delete folder if requested
  if keep == '0':
    shutil.rmtree('Run_' + folder)
    
  return


### function to start new acquisition
def acquisition(config,time,folder,queue):
  cmd = ['readout','-c', config,'-t', time, '-f', folder,'--start']
  ret = subprocess.Popen(cmd).wait()
  #print('OUTPUT FROM READOUT =',ret)
  #global returncode
  #returncode = ret
  #print('returncode =',returncode)
  if ret == 255:
    queue.put('255')
  return 



### main function 
def main(argv):

   #parsing args
   parser = argparse.ArgumentParser(description='Python script to start acq')
   parser.add_argument('-c','--config' , help='Config file',required=True)
   parser.add_argument('-t','--time'   , help='Time per acq',required=True)
   parser.add_argument('-f','--folder' , help='Folder name',required=True)
   parser.add_argument('-k','--keep'   , help='0 = discard raw data; 1 = keep raw data',required=True)
   args = parser.parse_args()

   #print values
   print ("Config file      = %s" % args.config )
   print ("Time per acq [s] = %s" % args.time )
   print ("Folder name      = %s" % args.folder )
   if args.keep == '0':
     print ("Raw data will be discarded")
   else:
     print ("Keeping row data")
   
   #set initial acq slice counter
   counter = 0

   #make RootTTrees folder
   finalFolder = "Run_" + args.folder + "/RootTTrees"
   os.makedirs(finalFolder)

   #copy config file into main acq folder
   copyConfiFile = "Run_" + args.folder + "/" + args.config
   shutil.copyfile(args.config, copyConfiFile)
   
   #prepare a run controller
   run = 1
   # prepare first folder name
   folder = args.folder + "/" + str(counter)
   
   #start first slice of acq
   #returncode = acquisition(args.config,args.time,folder)
   cmd = ['readout','-c', args.config,'-t', args.time, '-f', folder,'--start']
   returncode = subprocess.Popen(cmd).wait()
   
   #stop if error code is returned (either because of an acq error or because the user press 'q')
   if returncode == 255:
     run = 0
   
   # when first acq is over, start an infinite loop of parallel conversion of previous acq and new acq
   while run == 1 :
     #convert old data
     eventsFile = 'Run_' + folder + '/events.dat'
     #read start time before converting data 
     fileStartTime = open("startTime.txt", "r") 
     startTime = fileStartTime.readline()
     fileStartTime.close()
     
     processList = []    
     pqueue = Queue()
     convertProcess = multiprocessing.Process(target=convert,args=(eventsFile,folder,args.folder,counter,startTime,args.keep))
     processList.append(convertProcess)
     
     counter = counter + 1
     folder = args.folder + "/" + str(counter)
     
     runProcess     = multiprocessing.Process(target=acquisition,args=(args.config,args.time,folder,pqueue))
     processList.append(runProcess)
     
     #print(returncode)
     
     for i in range(len(processList)):
       processList[i].start()
     
     #print(returncode)
     for i in range(len(processList)):
       processList[i].join()
     
     msg = pqueue.get()
     
     #print(returncode)
     
     if msg == '255':
       run = 0

   #convert last data
   eventsFile = 'Run_' + folder + '/events.dat'
   fileStartTime = open("startTime.txt", "r") 
   startTime = fileStartTime.readline()
   fileStartTime.close()
   print("Converting latest frame...\n")
   convert(eventsFile,folder,args.folder,counter,startTime,args.keep)
   
   print("Done!\n")


if __name__ == "__main__":
   main(sys.argv[1:])
