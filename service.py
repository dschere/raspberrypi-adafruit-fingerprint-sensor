#!/usr/bin/env python3
""" Service daemon for the fingerprint module. 

MQTT 

topic: 

device/fingerprint/request
    payload {
        method: <enroll>|<search>|<delete>|<query>
        [reqId:] 
    }   

device/fingerprint/response
    payload {
        event: <request-finger-press>|
               <request-finger-release>|
               <enrolled>|
               <fail>|
               <match> 
    }

"""
import fpsensor
import time
import sys
from   perif.deviceIface import DeviceHandler, AbstractDevice
import json

RESP_GRP="device/fingerprint/response"

class FingerprintSensor( AbstractDevice ):
    def name(self):
        return "fingerprint"
    def methods(self):
        return ["enroll","search","delete"]

    def __init__(self):
        AbstractDevice.__init__(self)
        fpsensor.setup()

    def _req_finger_press(self, mqttc, jobj):
        mqttc.publish(RESP_GRP,payload=json.dumps({
            "event":"request-finger-press",
            "respId": jobj.get('respId')
        }) 

    def _req_finger_release(self, mqttc, jobj):
        mqttc.publish(RESP_GRP,payload=json.dumps({
            "event":"request-finger-release",
            "respId": jobj.get('respId')
        }) 

    def enroll(self, mqttc, jobj):
        def msg_handler( msg ):
            if msg == "press finger":
                self._req_finger_press(mqttc, jobj)
            elif msg == "press release":
                self._req_finger_release(mqttc, jobj)
                

    def search(self, mqttc, jobj):
        return fpsensor.matchModel()
        





 



