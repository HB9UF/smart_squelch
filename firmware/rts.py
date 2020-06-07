import time
import serial

# ttyUSB0.rts : boot0 F030

ser = serial.Serial('/dev/ttyUSB0')
ser.setRTS(True)
time.sleep(10)
