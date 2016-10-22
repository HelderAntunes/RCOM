#pragma once

#define C_DATA 0x01
#define C_START 0x02
#define C_END 0x03

#define FILE_SIZE 0x00
#define FILE_NAME 0x01

#define MAX_PKT_SIZE 500

typedef struct {
	int fd;
	int mode;
	FILE * file;
	int pktSize;
} ApplicationLayer;

ApplicationLayer al;

/*
arguments:
- port serial port /dev/ttyS0 or /dev/ttyS1
- mode: TRANSMITTER / RECEIVER
- filePath: path to file
- timeout: number of secons before retransmitions
- pktSize: maximum size of package
- baurate: rate of transmition
returns:
0 if successful
-1 otherwise
*/
int initAppLayer(char * port, int mode, char * filePath, int timeout, int retries, int pktSize, int baudrate);
