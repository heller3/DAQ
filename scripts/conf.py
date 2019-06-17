import serial
import time

LANGUAGE = 'EN' # Ce genre de fonctionnalité utile LOL

if LANGUAGE == 'EN' :
    ENABLE_TEXT = 'Enable'
    DISABLE_TEXT = 'Disable'
    SEND_TEXT = 'Send'
    VOLTAGE_TITLE = 'Voltage (in V)'
    TITLE='SIPM Power Module A7585DU'

elif LANGUAGE == 'FR' :
    ENABLE_TEXT = 'Activer'
    DISABLE_TEXT = 'Désactiver'
    SEND_TEXT = 'Envoyer'
    VOLTAGE = 'Voltage (en V)'
    TITLE='SIPM Power Module A7585DU'

dac_labels = ["Voltage"]

row_labels = dac_labels
# Component configuration

initial_Voltage=19.001





ser = serial.Serial(

    port='/dev/vSupply0',
    baudrate = 115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
    )




# serial port numeration can change unpredictably (even during measurement)
# that's why the standard port='/dev/ttyUSB0' would fail at some point. 
# the solution can be found here:
# https://rolfblijleven.blogspot.com/2015/02/howto-persistent-device-names-on.html
# basically, it means creating a file 
# /etc/udev/rules.d/99-usb-serial.rules
# with
# SUBSYSTEM=="tty", ATTRS{idVendor}=="12d1", ATTRS{idProduct}=="1003", SYMLINK+="vSupply0"
# where the values for idVendor and idProduct can be found with 
# lsusb 
# for example 
# Bus 007 Device 014: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC
# which means idVendor = 0403 and idProduct = 6001
# then with
# sudo udevadm trigger
# check with
# ls -l /dev/vSupply0
# lrwxrwxrwx 1 root root 7 Apr 29 17:49 /dev/vSupply0 -> ttyUSB0

