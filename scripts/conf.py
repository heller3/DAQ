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

    port='/dev/ttyUSB0',
    # port='/dev/ttyAMA0',
    baudrate = 115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
    )
