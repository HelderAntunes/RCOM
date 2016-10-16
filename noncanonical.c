/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAME_SIZE 1024

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define F 0x7e
#define A 0x03

#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_0 0x00
#define C_1 0x40
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81

#define BCC(A,C) A^C

#define ESC 0x7c

volatile int STOP=FALSE;

int receiveFrame(int fd, unsigned char* frame){
	int i = 0;
	int state = 0;
	int receiving = TRUE;
	
	unsigned char c;
	
	while(receiving){
		read(fd, &c, 1);
		printf("0x%02x ", c);
		
		switch(state){
		case 0:
			if(c == F){
				frame[i] = c;
				i++;
				state++;
			}
			break;
		case 1:
			if(c == A){
				frame[i] = c;
				i++;
				state++;
			}
			else if (c != F){
				state = 0;
				i = 0;
			}
			break;
		case 2:
			if(c != F){
				frame[i] = c;
				i++;
				state++;
			}
			else{
				state = 1;
				i = 1;
			}
			break;
		case 3:
			if(c == BCC(frame[1],frame[2])){
				frame[i] = c;
				i++;
				state++;
			}
			else if(c == F){
				state 1;
				i = 1;
			}
			else{
				state 0;
				i = 0;
			}
			break;
		case 4:
			if(c == F){
				frame[i] = c;
				i++;
				receiving = FALSE;
			}
			else{
				frame[i] = c;
				i++;
			}
			break;
		default:
			break;
		}
	}
	return i;
}

int sendFrame(int fd, unsigned c){
	unsigned char frame[5]; 
	
	frame[0] = F;
	frame[1] = A;
	frame[2] = c;
	frame[3] = BCC(frame[1], frame[2]);
	frame[4] = F;

	int w = write(fd, frame, 5);
	printf("%d bytes written\n", w);
	
	if(w != 5){
		printf("ERROR in sendFrame(): could not send frame\n");
		return -1;
	}
	
	return 0;
}

int destuffFrame(unsigned char* frame, int frameSize, unsigned char* destuffedFrame){
	
	int i;
	int j = 0;
	for (i = 0; i < size; i++, j++) {
		if (df.frame[i] == ESC)
			destuffedFrame.frame[j] = df.frame[++i] ^ 0x20;
		else
			destuffedFrame.frame[j] = df.frame[i];
	}
	
	return j;
}

unsigned char getBCC2(unsigned char* data, unsigned int size) {
	unsigned char BCC = 0;

	int i;
	for (i = 0; i < size; i++)
		BCC ^= data[i];

	return BCC;
}

int llopen(int fd) {
	unsigned char frame[5];
	
	receiveFrame(fd, frame);
	
	/*int i = 0;
	int res;
	int index = 0;
	for(;i < 5;i++) {
		res = read(fd, frame + i,1);
		printf("0x%02x ", frame[i]);

		switch(index){
		  case 0: //reading F
		    if(frame[index] == F)
		      index++;
		    break;
		  case 1: //reading A
		    if(frame[index] == A)
		      index++;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 2: //reading C
		    if(frame[index] == C_SET)
		      index++;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 3: //reading BCC
		    if(frame[index] == A^C_SET)
		      index++;
		    else if(frame[index] == C_SET)
		      index = 3;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 4: //reading F
		    if(frame[index] == F)
		      STOP = TRUE;
		    break;
		  default:
		    break;
		}
	 
	}
	printf("\n");
	*/

	sendFrame(fd, C_UA);
	/*frame[0] = F;
	frame[1] = A;
	frame[2] = C_UA;
	frame[3] = BCC(frame[1], frame[2]);
	frame[4] = F;

	int w = write(fd, frame, 5);
	printf("%d bytes written\n", w);*/
	
}

int llread(int fd, char * buffer){
	unsigned char frame[MAX_FRAME_SIZE];
	unsigned char destuffedFrame[MAX_FRAME_SIZE];
	
	//Assuming that information is being received!!!!
	//TODO: Disconnect has to be processed
	int size = receiveFrame(fd, frame);
	
	if(size == 5){//frame is command
		if(frame[2] = C_DISC)
			return -1; //DISCONECT
		else
			return -2; //ERRO
	}
	
	int destuffedSize = destuffFrame(frame, size, destuffedFrame);
	
	int dataSize = destuffedSize - 6; //6 bytes are used in prefix and posfix
	
	unsigned char BCC2 = getBCC2(destuffFrame[4], dataSize);
	
	if(destuffedFrame[destuffedSize-2] != BCC2){
		printf("ERROR in receiveFrame(): BCC2 error\n");
		//Reject
		if(destuffedFrame[2] == C_0)
			sendFrame(fd, C_REJ0);
		else if(destuffedFrame[2] == C_1)
			sendFrame(fd, C_REJ1);
	}
	else{
		//Send ReceiverReady
		if(destuffedFrame[2] == C_0)
			sendFrame(fd, C_RR1);
		else if(destuffedFrame[2] == C_1)
			sendFrame(fd, C_RR0);
		
	}
	
	buffer = &destuffedFrame[4];
	buffer[destuffedSize-2] = '\0';
	
	return dataSize;
	
	/* //Read trama I
	int i = 0;
	int res;
	int index = 0;
	for(;i < 5;i++) {
		res = read(fd, frame + i,1);
		printf("0x%02x ", frame[i]);

		switch(index){
		  case 0: //reading F
		    if(frame[index] == F)
		      index++;
		    break;
		  case 1: //reading A
		    if(frame[index] == A)
		      index++;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 2: //reading C
		    if(frame[index] == C_0 || frame[index] == C_1)
		      index++;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 3: //reading BCC
		    if(frame[index] == BCC(frame[1],frame[2]))
		      index++;
		    else if(frame[index] == frame[index] == C_0 || frame[index] == C_1)
		      index = 3;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		//Read stuffed data
		}
	}*/
	
}

int llclose(int fd){
	unsigned char frame[MAX_FRAME_SIZE];
	
	sendFrame(fd, C_DISC);
	receiveFrame(fd, frame);
	
	if(frame[2] = C_UA)
		return 0;
	else{
		printf("ERROR in llclose(): did not receive UA frame\n");
		return -1;
	}
	
	/*//Sending DISC
	unsigned char frame[5]; 
	frame[0]=F;
	frame[1]=A;
	frame[2]=C_DISC;
	frame[3]=BCC(A,C_DISC);
	frame[4]=F;

	int w = write(fd, frame, 5);
	printf("%d bytes written\n", w);

	//Receiving UA
	int i = 0;
	int res;
	int index = 0;
	for(;i < 5;i++) {
		res = read(fd, frame + i,1);
		printf("0x%02x ", frame[i]);

		switch(index){
		  case 0: //reading F
		    if(frame[index] == F)
		      index++;
		    break;
		  case 1: //reading A
		    if(frame[index] == A)
		      index++;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 2: //reading C
		    if(frame[index] == C_UA)
		      index++;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 3: //reading BCC
		    if(frame[index] == BCC(A,C_UA))
		      index++;
		    else if(frame[index] == C_UA)
		      index = 3;
		    else if(frame[index] == A)
		      index = 2;
		    else if(frame[index] == F)
		      index = 1;
		    else
		      index = 0;
		    break;
		  case 4: //reading F
		    if(frame[index] == F)
		      STOP = TRUE;
		    break;
		  default:
		    break;
		}
	 
	}
	printf("\n");*/
	
}

int main(int argc, char** argv)
{
    int fd,c, res, index;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
      
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    tcgetattr(fd,&oldtio); /*m save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    printf("New termios structure set\n");
	
	llopen(fd);

    sleep(15);
	
	//llclose(fd);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
