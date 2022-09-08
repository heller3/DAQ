import time
import serial
import struct


# Send data to stage
def send(stages,device, command, data=0):
   # send a packet using the specified device number, command number, and data
   # The data argument is optional and defaults to zero
	packet = struct.pack('<BBl', device, command, data)
	stages.write(packet)

# Receive data from stages
def receive(stages):
   # return 6 bytes from the receive buffer
   # there must be 6 bytes to receive (no error checking)
    r = [0,0,0,0,0,0]
    for i in range (6):
        r[i] = ord(stages.read(1))
    return r

# Traslate position returned by stage into mm
def translate_received_position(position):
	step_size   = 0.49609375 #micrometers
	# translate
	reply_data = pow(256,3) * position[5] + pow(256,2) * position[4] + 256 * position[3] + position[2]
	if position[5] > 127:
		reply_data = reply_data - pow(256,4)
	reply_data = reply_data*step_size
	return reply_data

def translate_firmware_version(position):
	reply_data = pow(256,3) * position[5] + pow(256,2) * position[4] + 256 * position[3] + position[2]
	return reply_data

# Move stage of a relative distance, expressed in microns
def move_stage(stages,device,distance):
    time.sleep(1)
    command     = 21         #move relative
    data = translate_target_position(distance)
    send(stages,device,command,data)
    ret = stages.readline()
    if ret[1] == 255:
      print('ERROR! Stage returned error code 255. No movement.')
    else:
      print('Success')

def translate_target_position(position):
    step_size   = 0.49609375 #micrometers
    N_steps   = int(position/step_size)
    return N_steps

# Move stage of a relative distance, expressed in microns
def move_stage_absolute(stages,device,position):
    # time.sleep(1)
    command     = 20         #move relative
    data = translate_target_position(position)
    send(stages,device,command,data)
    ret = stages.readline()
    if ret[1] == 255:
      print('ERROR! Stage returned error code 255. No movement.')
    else:
      print('Success')

def get_firmware_version(stages):
	command=51
	data=0
	device=0
	send(stages,device,command,data)
	fw_version = receive(stages)
	return fw_version

# Get stage current position
def get_current_position(stages,device):
    command=60
    data=0
    send(stages,device,command,data)
    position = receive(stages)
    ret_position = translate_received_position(position)
    return ret_position

# Get stage stored position
def get_stored_position(stages,device):
    command=17
    data=0
    send(stages,device,command,data)
    position = receive(stages)
    ret_position = translate_received_position(position)
    return ret_position

def send_both_home(stages):
    command=1
    data=0
    device=0
    send(stages,device,command,data)

def send_both_to_stored_position(stages):
    command=18
    data=0
    device=0
    send(stages,device,command,data)

def store_positions(stages):
    command=16
    data=0
    device=0
    send(stages,device,command,data)

def set_maximum_position(stages,device,position):
    command = 44 
    data = translate_target_position(position)
    send(stages,device,command,data)

def get_maximum_position(stages,device):
    command = 53
    data = 44 
    send(stages,device,command,data)
    position = receive(stages)
    ret_position = translate_received_position(position)
    return ret_position

######################
# PRINT
######################

# Print current position of both stages
def print_current_positions(stages):
    print ('#################################')
    print ('# CURRENT POSITION              #')
    print ('#################################')
    device=2 # aka x
    x_position = get_current_position(stages,device)
    print("Current x [microns] = %f" %(x_position))
    device=1 # aka z
    z_position = get_current_position(stages,device)
    print("Current z [microns] = %f" %(z_position))
    print ('#################################\n')

# Print stored position of both stages
def print_stored_positions(stages):
    print ('#################################')
    print ('# STORED POSITION               #')
    print ('#################################')
    device=2 # aka x
    x_position = get_stored_position(stages,device)
    print("Stored x [microns] = %f" %(x_position))
    device=1 # aka z
    z_position = get_stored_position(stages,device)
    print("Stored z [microns] = %f" %(z_position))
    print ('#################################\n')

def print_firmware_version(stages):
    fw_version = get_firmware_version(stages)
    print ('#################################')
    print ('# FIRMWARE VERSION              #')
    print ('# xxx means version x.xx        #')
    tr_fw = translate_firmware_version(fw_version)
    print ('# %i          ' %tr_fw)
    print ('#################################')

def print_maximum_positions(stages):
    print ('#################################')
    print ('# MAXIMUM POSITIONS             #')
    print ('#################################')
    device=2 # aka x
    x_position = get_maximum_position(stages,device)
    print("Max x [microns] = %f" %(x_position))
    device=1 # aka z
    z_position = get_maximum_position(stages,device)
    print("Max z [microns] = %f" %(z_position))
    print ('#################################')
