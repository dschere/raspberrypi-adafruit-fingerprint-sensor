#ifndef ENROLL_H
#define ENROLL_H


uint8_t getFingerprintEnroll(
    HardwareSerial& Serial, Adafruit_Fingerprint& finger, 
    uint8_t id,
    void (*emit)(const char* event) 
);


#endif

