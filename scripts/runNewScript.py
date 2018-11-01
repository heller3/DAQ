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
   
   #make RootTTrees folder 
   finalFolder = "Run_" + args.folder + "/RootTTrees"
   os.makedirs(finalFolder)
   
   #copy config file into main acq folder
   copyConfiFile = "Run_" + args.folder + "/" + args.config
   shutil.copyfile(args.config, copyConfiFile)  
   

   run = 1
   folder = args.folder + "/" + str(counter)
   
   
   
   
if __name__ == "__main__":
   main(sys.argv[1:])