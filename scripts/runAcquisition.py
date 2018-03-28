#!/usr/bin/python3
# -*- coding: utf-8 -*-


import math
import os
import stat
import sys
import argparse
import subprocess
from subprocess import Popen, PIPE, STDOUT

def main(argv):

   #parsing args
   parser = argparse.ArgumentParser(description='Python script to start acq')
   parser.add_argument('-c','--config' , help='Config file',required=True)
   parser.add_argument('-t','--time'   , help='Time per acq',required=True)
   parser.add_argument('-f','--folder' , help='Folder name',required=True)
   args = parser.parse_args()

   #print values
   print ("Config file      = %s" % args.config )
   print ("Time per acq [s] = %s" % args.time )
   print ("Folder name      = %s" % args.folder )
   
   counter = 0

   run = 1
   folder = args.folder + "/" + str(counter)
   cmd = ['readout','-c', args.config,'-t', args.time, '-f', folder,'--start']
   returncode = subprocess.Popen(cmd).wait()
     #print(child.returncode)
   if returncode == 255:
     run = 0
   
   fName = "convert.sh"
   
   while run == 1 :
     #convert old data
     eventsFile = 'Run_' + folder + '/events.dat'
     #create script
     
     f = open(fName,'w')
     f.write("#!/bin/bash\n")
     f.write("events -o %s " %eventsFile)
     f.write("--input0 Run_%s/binary740.dat "   %folder)
     f.write("--input1 Run_%s/binary742_0.dat " %folder)
     f.write("--input2 Run_%s/binary742_1.dat " %folder)
     f.write(" && convertToRoot ")
     f.write("--input %s "   %eventsFile)
     f.write("--output-folder Run_%s " %folder)
     f.write("--frame %d " %counter)
     f.write(" && rm Run_%s/binary740.dat " %folder )
     f.write(" && rm Run_%s/binary742_0.dat " %folder )
     f.write(" && rm Run_%s/binary742_1.dat " %folder )
     f.write(" && rm %s " %eventsFile )
     f.close()
     #and make it executable
     st = os.stat(fName)
     os.chmod(fName, st.st_mode | stat.S_IEXEC)
     
     convertCmd = ['./' + fName]
     convertCode = subprocess.Popen(convertCmd,shell=True).poll()
     print(convertCode)
     
     #start the next acq
     counter = counter + 1
     folder = args.folder + "/" + str(counter) 
     cmd = ['readout','-c', args.config,'-t', args.time, '-f', folder,'--start']
     returncode = subprocess.Popen(cmd).wait()
     
     #print(child.returncode)
     if returncode == 255:
       run = 0
   
   #convert last data
   #convert old data
   eventsFile = 'Run_' + folder + '/events.dat'
   #create script
   f = open(fName,'w')
   f.write("#!/bin/bash\n")
   f.write("events -o %s " %eventsFile)
   f.write("--input0 Run_%s/binary740.dat "   %folder)
   f.write("--input1 Run_%s/binary742_0.dat " %folder)
   f.write("--input2 Run_%s/binary742_1.dat " %folder)
   f.write(" && convertToRoot ")
   f.write("--input %s "   %eventsFile)
   f.write("--output-folder Run_%s " %folder)
   f.write("--frame %d " %counter)
   f.write(" && rm Run_%s/binary740.dat " %folder )
   f.write(" && rm Run_%s/binary742_0.dat " %folder )
   f.write(" && rm Run_%s/binary742_1.dat " %folder )
   f.write(" && rm %s " %eventsFile )
   f.close()
   #and make it executable
   st = os.stat(fName)
   os.chmod(fName, st.st_mode | stat.S_IEXEC)
   
   convertCmd = ['./' + fName]
   convertCode = subprocess.Popen(convertCmd,shell=True).wait()
   
   print("done!\n")


if __name__ == "__main__":
   main(sys.argv[1:])

