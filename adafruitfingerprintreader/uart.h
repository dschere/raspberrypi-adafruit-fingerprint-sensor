#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>             //Used for UART
#include <fcntl.h>              //Used for UART
#include <termios.h>        //Used for UART

#define boolean unsigned char
#define BYTE 1

class HardwareSerial
{
    int uart0_filestream;
    struct termios cfg;    
public:

    HardwareSerial();

    void begin(unsigned long baud);
    void print(uint8_t ch, int sz);
    void write(uint8_t ch);
    int read();    
    int available();
    
    
    int error;

    int open(char* device);
    void close();
    
};




#endif

