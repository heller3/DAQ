#!/usr/bin/python
# -*- coding: utf-8 -*-


import math
import os
import stat
import sys
import argparse
import subprocess
from subprocess import Popen, PIPE, STDOUT
import shutil
import time



def main(argv):
  
  #"runAcquisition.py --config config.txt --time 1800 --folder th2_0.30"
  #cmd = ['runAcquisition.py','--config', 'config.txt','--time','1800', '--folder','prova']
  cmd = ['readout','-c', 'config.txt','-t','1800', '-f', 'prova2','--start']
  #cmd = ['readout','--config', 'config.txt','--time','1800', '--folder','prova']
  ps = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr = subprocess.PIPE, stdin=subprocess.PIPE)
  time.sleep(60)
  ps.stdin.write('q')
  #chunck = ps.stdout.read()
  
  ps = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr = subprocess.PIPE, stdin=subprocess.PIPE)
  
if __name__ == "__main__":
  main(sys.argv[1:])
