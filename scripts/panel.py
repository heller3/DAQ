#!/usr/bin/python3
# -*- coding: utf-8 -*-

'''
Coded by Louis Berry & Théophile Blard, 2018
'''

#### Il faut toujours send la valeur des DAC après avoir update Vref
#### Apres un reset il faut a nouveau send les valeurs des DAC


import time
from conf import *
from gui import Row, MainWindow


class Component:
    def __init__(self, initial_Voltage):
        #keep initial values for reset
        self.initial_Voltage = initial_Voltage
        #Valeur qui sort de la raspberry
        self.Voltage = [initial_Voltage]*1


    def send_value(self, index, value):
        # print(index)
        try:
            float_value = string_to_float(value)
        except:
            return
        if(index==0):

            sent_value = self.change_dac_value(index, float_value)
            send_Volt(self.Voltage[0])
        return sent_value


    # Update 1 value with the value as an argument
    def change_dac_value(self, index, value):

        if(index==0): # A,B,C,D
            if(value>=84):
                value = 83.999
                #NOTE : why O.O01 and not 0 ? If important, store it in the conf file
            # low cutoff not needed here!
            #else: # E,F,G,H
            #    if(value<=19):
            #        value = 19.001

        #update the list
        self.Voltage[index] = value

        #
        # print("SUCCESS : Value: "+str(value))
        # print('Voltage value = ',self.Voltage)
        # print('')

        return value


    def reset(self):
        self.Voltage = [initial_Voltage]*1


        # Send the updates
        self.update_dac_values()
        send_Volt(self.Voltage[0])
        print("SUCCESS : All values were reseted")



    # Update all values according to the list
    def update_dac_values(self):
        for i in range(0, 1):
            self.send_value(i, self.Voltage[i])

    def switch_on_hv(self):
        hv_on()

    def switch_off_hv(self):
        hv_off()

    def read_voltage(self):
        value = read_V()
        return value

    def read_current(self):
        value = read_I()
        return value

    def begin_com(self):
        initial_com()

#####

def initial_com():
    # one sleep
    time.sleep(0.10)
    # first AT command is needed
    ser.write(b'AT\n')
    time.sleep(0.10)
    wasteBin = ser.readline().decode('ascii')
    # set comunication to machine mode
    ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    time.sleep(0.10)
    wasteBin = ser.readline().decode('ascii')

    # set digital mode
    ser.write(b'AT+SET,1,0\n')
    time.sleep(0.10)
    # print ("Setting to Digital mode... ", ser.readline().decode('ascii'))
    wasteBin = ser.readline().decode('ascii')

def read_V():
    # # one sleep
    # time.sleep(0.10)
    # # first AT command is needed
    # ser.write(b'AT\n')
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # # set comunication to machine mode
    # ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')

    ser.write(b'AT+GET,231\n')
    time.sleep(0.10)
    readOut = ser.readline().decode('ascii')
    # print ("V [Volt] = ", readOut[3:], end="\r")
    value = float(readOut[3:])
    return value

def read_I():
    # # one sleep
    # time.sleep(0.10)
    # # first AT command is needed
    # ser.write(b'AT\n')
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # # set comunication to machine mode
    # ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')

    ser.write(b'AT+GET,232\n')
    time.sleep(0.10)
    readOut = ser.readline().decode('ascii')
    # print ("V [Volt] = ", readOut[3:], end="\r")
    value = float(readOut[3:])
    return value


def hv_on():
    # one sleep
    # time.sleep(0.10)
    # # first AT command is needed
    # ser.write(b'AT\n')
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # # set comunication to machine mode
    # ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # set digital mode
    # ser.write(b'AT+SET,1,0\n')
    # time.sleep(0.10)
    # # print ("Setting to Digital mode... ", ser.readline().decode('ascii'))
    # wasteBin = ser.readline().decode('ascii')

    #enable HV
    ser.write(b'AT+SET,0,1\n') ##############################################
    time.sleep(0.10)
    print ("Enabling HV...", ser.readline().decode('ascii') )
    # wasteBin = ser.readline().decode('ascii')
    time.sleep(0.10)

def hv_off():
    # one sleep
    # time.sleep(0.10)
    # # first AT command is needed
    # ser.write(b'AT\n')
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # # set comunication to machine mode
    # ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii')
    # # set digital mode
    # ser.write(b'AT+SET,1,0\n')
    # time.sleep(0.10)
    # # print ("Setting to Digital mode... ", ser.readline().decode('ascii'))
    # wasteBin = ser.readline().decode('ascii')

    #enable HV
    ser.write(b'AT+SET,0,0\n') ##############################################
    time.sleep(0.10)
    print ("Disabling HV...", ser.readline().decode('ascii') )
    # wasteBin = ser.readline().decode('ascii')
    time.sleep(0.10)




def string_to_float(string_value):

    # no need to test the type because the GUI only outputs string values
    # replace comma by dots, if needed
    string_value = string_value.replace(',', '.')

    try:
        output=float(string_value)
        return output
    except ValueError as error:
        print ("ERROR : Value not sent. Please enter a float number")
        raise error


def send_Volt(Voltage):
    v_target = float(Voltage)
    Voltage=str(Voltage)
    Voltage = Voltage.encode('utf-8')

    # one sleep
    time.sleep(0.10)

    # first AT command is needed
    # ser.write(b'AT\n')
    # time.sleep(0.10)
    # wasteBin = ser.readline().decode('ascii') #this should just reply ERROR, and it needs to be sent first
    #
    # # set comunication to machine mode
    # ser.write(b'AT+MACHINE\n') #doesn't reply apparently...
    # time.sleep(0.10)
    #
    #
    # # set digital mode
    # ser.write(b'AT+SET,1,0\n')
    # time.sleep(0.10)
    # # print ("Setting to Digital mode... ", ser.readline().decode('ascii'))
    # wasteBin = ser.readline().decode('ascii')

    #set max voltage
    ser.write(b'AT+SET,4,83.99999,\n')
    time.sleep(0.10)
    # print ("Setting MAX voltage to 84 V... ", ser.readline().decode('ascii'))
    wasteBin = ser.readline().decode('ascii')

    # set target voltage
    ser.write(b'AT+SET,2,'+Voltage+b'\n')
    time.sleep(0.10)
    # print ("Setting TARGET voltage to ", Voltage )
    # print ("", ser.readline().decode('ascii'))
    wasteBin = ser.readline().decode('ascii')
    # vtarget = ser.readline().decode('ascii')

    # readOut = ser.readline().decode('ascii')
    # print ("Reply: ", readOut)

    #enable HV
    # ser.write(b'AT+SET,0,1\n') ##############################################
    # time.sleep(0.10)
    # # print ("Enabling HV...", ser.readline().decode('ascii') )
    # wasteBin = ser.readline().decode('ascii')

    # ser.write(b'AT+GET,2\r\n')
    # time.sleep(0.1)
    # readOut = ser.readline().decode('ascii')
    # print ("V target [Volt] = ", readOut[3:])

    # ser.write(b'AT+GET,231\n')
    # time.sleep(0.10)
    # readOut = ser.readline().decode('ascii')
    # print ("V [Volt] = ", readOut[3:])

    # print ("Ramping...")
    # loops = 0
    # while True:
    #     loops = loops + 1
    #     ser.write(b'AT+GET,231\n')
    #     time.sleep(0.10)
    #     readOut = ser.readline().decode('ascii')
    #     print ("V [Volt] = ", readOut[3:], end="\r")
    #     v_now = float(readOut[3:])
    #     if (v_target - 0.05) < v_now < (v_target + 0.05):
    #         break
    #     if loops > 99:
    #         break

    print('')
    print ("VOLTAGE SET TO TARGET ", v_target, " V")


    # ser.write(b'AT+GET,232\n')
    # time.sleep(0.10)
    # readOut = ser.readline().decode('ascii')
    # print ("Current [mA] = ", readOut[3:])


'''
Main program
'''

# This function is called when the script is launched (and not when its imported as a module)
if __name__ == "__main__":
    # Create the component object and initialize its values
    component = Component(initial_Voltage)

    #GUI
    # Create the window
    main_window = MainWindow(component)

    # Display it on the screen
    main_window.make_layout(row_labels)
