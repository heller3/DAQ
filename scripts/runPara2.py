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
import tempRunAcquisition as tr


def main(argv):
  print("ciao")
  tr.main(argv)
  return 0


if __name__ == "__main__":
   main(sys.argv[1:])
