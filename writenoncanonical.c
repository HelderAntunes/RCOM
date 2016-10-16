/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define F 0x7e
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

volatile int STOP=FALSE;

int flag=1, conta=1;

void atende()                   // atende alarme
{
	printf("Try number: %d\n", conta);
	flag = 1;
	conta++;
}

void sendSET(int fd) {
    unsigned char SET[5];
	SET[0] = F;
	SET[1] = A;
	SET[2] = C_SET;
	SET[3] = SET[1] ^ SET[2];
	SET[4] = F;
	int w = write(fd, SET, 5);
	printf("%d bytes written\n", w);
}

void readUA(int fd) {
    int j= 0;
    unsigned char UA[5];
	int res;
	tcflush(fd, TCIFLUSH);
	for(; STOP != TRUE && flag == 0;) {
        res = read(fd, UA + j, 1);
        if(res < 1) continue;
		printf("0x%02x ", UA[j]);

		switch(j){
              case 0: //reading F
                if(UA[j] == F)
                  j++;
                break;
              case 1: //reading A
                if(UA[j] == A)
                  j++;
                else if(UA[j] == F)
                  j = 1;
                else
                  j = 0;
                break;
              case 2: //reading C
                if(UA[j] == C_UA)
                  j++;
                else if(UA[j] == A)
                  j = 2;
                else if(UA[j] == F)
                  j = 1;
                else
                  j = 0;
                break;
              case 3: //reading BCC
                if(UA[j] == A^C_UA)
                  j++;
                else if(UA[j] == C_UA)
                  j = 3;
                else if(UA[j] == A)
                  j = 2;
                else if(UA[j] == F)
                  j = 1;
                else
                  j = 0;
                break;
              case 4: //reading F
                if(UA[j] == F) {
                    STOP = TRUE;
                    printf("Message successful received!\n");
                }
                break;
            }
    }

}

int llopen(int fd) {

    while(conta < 4 && STOP != TRUE){

        if(flag){
            sendSET(fd);
	        alarm(3);
	        flag = 0;
	        readUA(fd);
        }
        
	}

}

int main(int argc, char** argv)
{
    int fd,c, res, len;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd, &oldtio) == -1) {
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;
    newtio.c_cc[VMIN]     = 1;

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    printf("New termios structure set\n");

	(void) signal(SIGALRM, atende);
	llopen(fd);


    sleep(2);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
