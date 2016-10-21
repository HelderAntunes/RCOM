#pragma once

#define C_DATA 1
#define C_START 2
#define C_END 3

#define FILE_SIZE 0
#define FILE_NAME 1

#define MAX_PKT_SIZE 100

typedef struct {
	int fd;
	int mode;
	FILE * file;
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

/*
arguments:
- filePath: path to file
- fileSize: size of file in chars
returns:
0 if successful
-1 otherwise
*/
int sendData(char * filePath, int fileSize);

/*
arguments:
- filePath: path to file
returns:
0 if successful
-1 otherwise
*/
int receiveData(char * filePath);

/*
arguments:
- ctrlField: value of first byte of packet
- filePath: path to file
- fileSize: size of file in chars
returns:
0 if successful
-1 otherwise
*/
int sendCtrlPkt(int ctrlField, char * filePath, int fileSize);
/*
arguments:
- ctrlField: value of first byte of packet
- filePath: path to file
- fileSize: size of file in chars
returns:
0 if successful
-1 otherwise
*/
int receivecvCtrlPkt(int ctrlField, int * fileSize, char * filePath);

/*
arguments:
- buffer: data received
- bytesRead: number of bytes read
- sequenceNumber: number in sequence (modulo 255)
returns:
0 if successful
-1 otherwise
*/
int sendDataPkt(char * buffer, int bytesRead, int sequenceNumber);
/*
arguments:
- buffer: data received
- sequenceNumber: number in sequence (modulo 255)
returns:
0 if successful
-1 otherwise
*/
int receiveDataPkt(unsigned char * buffer,int sequenceNumber);