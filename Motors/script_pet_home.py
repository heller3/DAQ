import minimalmodbus
import sys
import serial
import time

motor1 = minimalmodbus.Instrument('/dev/Motors', 1) # port name, slave address (in decimal)
motor2 = minimalmodbus.Instrument('/dev/Motors', 2) # port name, slave address (in decimal)
#motor1.debug = False
#motor2.debug = False


motor1.serial.baudrate = 9600
motor1.serial.bytesize = 8
motor1.serial.parity   = serial.PARITY_EVEN
motor1.serial.stopbits = 1
motor1.serial.timeout  = 0.1
motor1.mode = minimalmodbus.MODE_RTU

motor2.write_register(705,1,functioncode=16)

#motor1.write_register(125,32,functioncode=16)
#motor2.write_register(125,32,functioncode=16)
#time.sleep(2)

motor1.write_register(125,16,functioncode=16)
#time.sleep(20)
#motor2.write_register(125,32,functioncode=16)
