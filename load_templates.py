#!/usr/bin/env python3

import fpsensor
import time
import sys
import json
import os
import glob

sys.path.append("/usr/local/lib/python3.4/dist-packages/")
os.system("mkdir -p %s/.worldview/perif/fingerprint/data" % os.environ["HOME"])

fpsensor.setup()

spec = os.environ['HOME']+"/.worldview/perif/fingerprint/data/model-%d.json"
for ident in range(0,256):
    f = spec % ident
    if os.access( f, os.F_OK ):
        data = json.loads(open(f).read())
        fpsensor.uploadTemplate( ident, data )

