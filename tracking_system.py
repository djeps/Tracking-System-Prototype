import datetime
import os
import random
import sys
import serial
import signal
import subprocess
import time
import threading
import pifacedigitalio
import numpy

STATE_HIGH = 1
STATE_LOW = 0
INPUT1 = 0
INPUT2 = 1
OUTPUT1 = 0 # Connected to RELAY 1
OUTPUT2 = 1 # Connected to RELAY 2
OUTPUT3 = 2
SETTLE_TIME = 0.250
POWER_FAILURE_RESTORE_TIME = 5.000

def on_input0_high():
    os.system('clear')
    print("Input 0 is HIGH.")
    print("Wake me up before you go!")
    print("Going to sleep...")
    subprocess.call("shutdown +1", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def stop_sequence_detected(*args):
    global pfd
    os.system('clear')
    print("\nCtrl+Z detected!")
    print("Cleanup done and program terminated.")
    pfd.relays[OUTPUT1].value = STATE_LOW
    pfd.output_pins[OUTPUT3].value = STATE_LOW
    sys.exit()

def interrupt_sequence_detected(*args):
    global pfd
    os.system('clear')
    print("\nCtrl+C detected!")
    print("Cleanup done and program terminated.")
    pfd.relays[OUTPUT1].value = STATE_LOW
    pfd.output_pins[OUTPUT3].value = STATE_LOW
    sys.exit()

def poweroff_handler():
    global pfd
    print("Power wasn't restored in: %d (sec)!" % (POWER_FAILURE_RESTORE_TIME))
    pfd.relays[OUTPUT1].value = STATE_LOW
    pfd.output_pins[OUTPUT3].value = STATE_LOW
    on_input0_high()
    sys.exit()

signal.signal(signal.SIGTSTP, stop_sequence_detected)
signal.signal(signal.SIGINT, interrupt_sequence_detected)

one_shot_input0 = True

pfd = pifacedigitalio.PiFaceDigital()
pfd.relays[OUTPUT1].value = STATE_HIGH

os.system('clear')
print("[PiFace Digital board]: listening on input 0 for poweroff key press.")
print("[ttyACM0]: listening for incoming serial data.")

start_time_sec = 0.000
delta_time_sec = 0.000

comPort = serial.Serial("/dev/ttyACM0", 9600, timeout=SETTLE_TIME)
comPort.baudrate = 9600

gps_datetime = ""
gps_tanklevel = ""
gps_lat = ""
gps_lon = ""

try:
    while True:
        pfd.output_pins[OUTPUT3].value = STATE_LOW
        
        if pfd.input_pins[INPUT1].value == STATE_HIGH:
            if one_shot_input0:
                one_shot_input0 = False # Lock no. 1!
                print("Process timer started!")
                print("Counting for: %d (sec)." % (POWER_FAILURE_RESTORE_TIME))
                start_time_sec = time.time()

            delta_time_sec = time.time() - start_time_sec

            if delta_time_sec >= POWER_FAILURE_RESTORE_TIME:
                poweroff_handler()
                break
        else:
            one_shot_input0 = True
            
            if (delta_time_sec > 0) and (delta_time_sec < POWER_FAILURE_RESTORE_TIME):
                print("Process timer canceled.")
                print("Time elapsed: %f(sec)" % (delta_time_sec))
                delta_time_sec = 0

            buffer_comPort = comPort.readline()

            if len(buffer_comPort) > 0:
                readLine_comPort = buffer_comPort.decode('utf-8')
                print(readLine_comPort)

                if "#GPS_LAT=" in readLine_comPort:
                    gps_lat = (readLine_comPort.split('=')[1]).split(';')[0]
                
                if "#GPS_LON=" in readLine_comPort:
                    gps_lon = (readLine_comPort.split('=')[1]).split(';')[0]
                    now = datetime.datetime.now()
                    gps_datetime = str(now.hour) + ":" + str(now.minute) + ':' + str(now.second) + ' ' + str(now.day) + '.' + str(now.month) + '.' + str(now.year)
                    gps_tanklevel = str(random.randrange(70, 101, 2)) + '%'

                if (gps_lat != "") and (gps_lon != ""):
                    tracking_system_log = open('/home/morpheus/Logs/tracking_system.log','a')
                    tracking_system_log.write(gps_datetime + ';' + gps_tanklevel + ';' + gps_lat + ';' + gps_lon + '\r\n')
                    tracking_system_log.close()
                    print("Successfully writen to file!")
                    pfd.output_pins[OUTPUT3].value = STATE_HIGH
                    gps_lat = ""
                    gps_lon = ""
            
        time.sleep(SETTLE_TIME)

except Exception as e:
    print("\nAn exception was raised!")
    print("\nException message:\n%s\n" % (str(e)))
    print("Going to sleep...")
    os.system('./poweroff.sh')
    sys.exit()
