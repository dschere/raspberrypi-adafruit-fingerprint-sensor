#!/usr/bin/env python3

import fpsensor
import time
import sys
import json
import os

sys.path.append("/usr/local/lib/python3.4/dist-packages/")
os.system("mkdir -p %s/.worldview/perif/fingerprint/data" % os.environ["HOME"])

spec = os.environ['HOME']+"/.worldview/perif/fingerprint/data/model-%d.json"

fpsensor.setup()

for ident in range(0,256):
    data = fpsensor.getTemplate(ident, 0)
    if data:
        open(spec % ident,"w").write(json.dumps(data))
        sys.stdout.write("Saved template %d\n    " % ident)
        for (i,ch) in enumerate(data):
            if i % 16 == 0:
                sys.stdout.write("\n    ")
            sys.stdout.write(" 0x%02X " % ch)
        sys.stdout.write("\n")

