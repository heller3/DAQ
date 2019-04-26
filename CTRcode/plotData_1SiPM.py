#!/usr/local/bin/python3.6
# -*- coding: utf-8 -*-

#import ROOT
#from ROOT import TH2F
#from ROOT import TMath
#from glob import iglob
#loop over 1 SiPm with method 1
#import numpy as np
#from array import array
#from ROOT import kBlack, kBlue, kRed, kOrange, kGreen
import math as m
import argparse
import os
import stat
import sys
import glob
#from ROOT import TH1F, TH1, TSpectrum, TFile, TGraph
#from pprint import pformat
import matplotlib.pyplot as plt
import csv


def main():

    parser = argparse.ArgumentParser(description='Plot data')
    parser.add_argument('-i','--input'    , help='Data file'   ,required=True)
    parser.add_argument('-o','--output'   , help='Output file' ,required=True)
    parser.add_argument('-t','--title'   , help='Plot title' ,required=True)
    args = parser.parse_args()



    voltage = []
    ctr_uc = []
    ctr_c = []
    ctr_shift = []
    ctr_w = []

    with open(args.input,'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=' ')
        for row in plots:
           voltage.append(float(row[0]))
           ctr_uc.append(1e12*float(row[1]))
           ctr_c.append(1e12*float(row[2]))
           ctr_shift.append(1e12*float(row[3]))
           ctr_w.append(1e12*float(row[4]))


    plt.plot(voltage,ctr_uc, label='No Correction')
    plt.plot(voltage,ctr_c, label='Amplitude Correction')
    plt.scatter(voltage,ctr_uc)
    plt.scatter(voltage,ctr_c)

    plt.xlabel('Bias Voltage [V]')
    plt.ylabel('CTR FWHM [ps]')
    plt.title(args.title)
    plt.legend()
    plt.grid()
    plt.savefig(args.output)
    plt.show()





if __name__ == '__main__':
    main()
