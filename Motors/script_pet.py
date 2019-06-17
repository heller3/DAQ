import minimalmodbus
import sys
import serial
import time

motor1 = minimalmodbus.Instrument('/dev/ttyUSB1', 1) # port name, slave address (in decimal)
motor2 = minimalmodbus.Instrument('/dev/ttyUSB1', 2) # port name, slave address (in decimal)
#motor1.debug = False
#motor2.debug = False
 

motor1.serial.baudrate = 9600
motor1.serial.bytesize = 8
motor1.serial.parity   = serial.PARITY_EVEN
motor1.serial.stopbits = 1
motor1.serial.timeout  = 0.1
motor1.mode = minimalmodbus.MODE_RTU


def set_jog_step():
    #motor2.write_register(1025,375,functioncode=16)
    motor1.write_register(4169,375,functioncode=16)
    motor2.write_register(4169,375,functioncode=16)

def jog_both_arms():
    time.sleep(0.1)

    #set to STOP both arms, just to be sure
    motor1.write_register(125,32,functioncode=16)
    motor2.write_register(125,32,functioncode=16)
    time.sleep(0.1)
    
    
    
    
    #set to + and - JOG both arms
    motor2.write_register(125,4096,functioncode=16)
    motor1.write_register(125,8192,functioncode=16)
    
    
    
    
    #wait for both arms to stop
    time.sleep(0.5)
    # start acquisition
    #time.sleep(0.2)
    #set to STOP both arms
    #motor1.write_register(125,32,functioncode=16)
    #motor2.write_register(125,32,functioncode=16)

    #time.sleep(0.2)

def stop_both_arms():
    motor1.write_register(125,32,functioncode=16)
    motor2.write_register(125,32,functioncode=16)

set_jog_step()

time.sleep(1)

for x in range(10):
    print(x)
    jog_both_arms()
    stop_both_arms()
