#!/usr/bin/env python3

import fpsensor
import time

fpsensor.setup()

"""
print("ready: please press sensor, scanning in 3 seconds\n")
time.sleep(3)
fpsensor.captureImage(1)
print("release sensor\n")
fpsensor.fingerRelease()
print("press sesnor, scanning in 3 seconds\n")
time.sleep(3)
fpsensor.captureImage(2)
print("processing ... \n")
fpsensor.createModel(10)
"""

"""
data = fpsensor.getTemplate(10)
import sys
sys.stdout.write("\n")
for (i,ch) in enumerate(data):
    if i % 16 == 0:
        sys.stdout.write("\n")
    sys.stdout.write(" 0x%02X " % ch)
"""

print("\nattempting to match finger print, scan in 3 seconds")
time.sleep(3)
print("before capture image\n");
fpsensor.captureImage()
print("after capture image, match model\n")
r = fpsensor.matchModel(10)
print("match model\n")
print(str(r))



