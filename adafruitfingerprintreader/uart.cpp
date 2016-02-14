#include "uart.h"

#include <poll.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>



/*

class HardwareSerial
{
    int uart0_filestream;
    struct termios cfg;
public:

    HardwareSerial();

    void begin(unsigned long baud);
    void print(uint8_t ch, int sz);
    int read();
};

class FakeSerial
{
public:
    void println(char* msg);
    void print(char *msg);
}


*/



int  HardwareSerial::read()
{
    unsigned char b=0;
    
    int len = ::read( uart0_filestream, (void*) &b, 1 );
    if ( len != 1 ) {
        fprintf(stderr,"Unable to read from uart!\n");
        fprintf(stderr,"errno %d, %s\n", errno, strerror(errno));
        error = 1;  
    }

#ifdef DEBUG
printf("recv --> len=%d, 0x%02X\n", len, b);
#endif
    return (len == 1)? (int)b: 0;
}

void HardwareSerial::print(const char* msg) 
{ 
    printf("%s",msg); 
}

void HardwareSerial::println(const char* msg)
{
    printf("%s\n", msg);
}

void HardwareSerial::println(uint8_t msg)
{
    printf("%d\n", msg);
}


void HardwareSerial::print(uint8_t ch, int sz)
{
    int r;
    r = ::write(uart0_filestream, &ch, 1);
    if ( r != 1 ) {
        fprintf(stderr,"Unable to write 0x%02d to uart\n",ch);
        fprintf(stderr,"errno %d, %s\n", errno, strerror(errno));
        error = 1;
    }

#ifdef DEBUG
printf("sending -> 0x%02X\n", ch);
#endif

}

void HardwareSerial::write(uint8_t ch) 
{
    print(ch,1);
}


int HardwareSerial::available()
{
   struct pollfd fds[1];
   int r;
   int active;

   fds[0].fd = uart0_filestream;
   fds[0].events = POLLIN;
   fds[0].revents = 0;

   r = poll( fds, 1, 0 );
   active = (r == 1) && (fds[0].revents & POLLIN);

   return active; 
}

void HardwareSerial::begin(unsigned long baudrate)
{
    if ( uart0_filestream == -1 ) {
        fprintf(stderr,"HardwareSerial::begin() not connected to uart");
        error = 1;
        return;
    }    

    //get existing configuration setup
    tcgetattr(uart0_filestream, &cfg);

    //fcntl(deviceFD, F_SETFL, FNDELAY);
    fcntl(uart0_filestream, F_SETFL, 0);

    // 9600, 19200, 28800, 38400, 57600 (default is 57600)
    switch( baudrate ) 
    {
    //TODO: make configurable 
    default:
        cfsetispeed( &cfg, B57600 );
        break; 
    } 

    cfg.c_cflag |= (CLOCAL | CREAD);

    ////8N1 (8 data bits, No parity, 1 stop bit)
    cfg.c_cflag &= ~PARENB;
    cfg.c_cflag &= ~CSTOPB;
    cfg.c_cflag &= ~CSIZE;
    cfg.c_cflag |= CS8;

    cfg.c_cflag &= ~CRTSCTS;  //~CNEW_RTSCTS; //disable hardware flow control

    //use RAW unbuffered data mode (eg, not canonical mode)
    cfg.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IGNBRK);

    cfg.c_iflag &= ~(IGNPAR | IXON | IXOFF | IXANY);

    //raw (unprocessed) output mode
    cfg.c_oflag &= ~OPOST;

    tcsetattr(uart0_filestream, TCSANOW, &cfg);

}


HardwareSerial::HardwareSerial()
{
    uart0_filestream = -1;
}

int HardwareSerial::open(char* device)
{
    if ((uart0_filestream = 
           ::open(device, O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
        fprintf(stderr,"Unable to open ttyAMA0 errno=%d %s",
            errno, strerror(errno)); 
    }
    return (uart0_filestream == -1) ? -1: 0;
}

void HardwareSerial::close() 
{
    if ( uart0_filestream != -1 ) {
        ::close( uart0_filestream );
        uart0_filestream = -1; 
    }
}





/*

// code is based on this raspberry pi UART example below: 

#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

int main(int argc, char* argv[])
{
//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream = -1;
	
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B57600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);



   return 0;
}
*/

