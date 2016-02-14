#!/usr/bin/env python3

import fpsensor
import time
import sys


def enroll( ident ):
    print("ready: please press sensor, scanning in 3 seconds\n")
    time.sleep(3)
    fpsensor.captureImage(1)
    print("release sensor\n")
    fpsensor.fingerRelease()
    print("press sensor, scanning in 3 seconds\n")
    time.sleep(3)
    fpsensor.captureImage(2)
    print("processing ...")
    fpsensor.createModel(ident)
    print("done.")

def show( ident ):
    data = fpsensor.getTemplate(ident)
    if not data:
        print("no match for %d" % ident)
        return

    sys.stdout.write("\n")
    for (i,ch) in enumerate(data):
        if i % 16 == 0:
            sys.stdout.write("\n")
        sys.stdout.write(" 0x%02X " % ch)
    print("\n") 

def match(  ):

    print("Attempting to match finger print, scan will start in 3 seconds")
    time.sleep(3)
    start = time.time()
    expire = start + 10
    while expire > time.time():
       
        try: 
            (matched,ident,confidence) = fpsensor.matchModel()
            if matched == 1:
                print("matched %d, confidence=%d" % (ident,confidence))
                return
        except:
            pass  

        time.sleep( 0.05 )


    print("timeout")

def delete( ident ):
    if ident == -1:
        for ident in range(1,255):
            try:  
                fpsensor.deleteModel(ident)
            except:
                pass
    else:
        fpsensor.deleteModel(ident)




if __name__ == '__main__':
    cmd = sys.argv[1]
    ident = -1
    if len(sys.argv) > 2:
        ident = int(sys.argv[2])
   
       
    fpsensor.setup()
    if cmd == "match":
        match( )
    elif cmd == "show":
        show( ident )
    elif cmd == "enroll":
        enroll( ident )
    elif cmd == "enroll2":
        def events( msg ):
            print("events::%s" % msg)

        r = fpsensor.getFingerprintEnroll( ident, events )
        print(r)
    elif cmd == "delete":
        delete( ident )
    else:
        print("Unknown command %s" % cmd)


