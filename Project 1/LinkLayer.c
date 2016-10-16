#include "LinkLayer.h"

volatile int STOP=FALSE;

int flag=1, conta=1;

void atende ();
int tryConnectInModeTransmitter (int fd);
void sendSET (int fd);
void readUA (int fd);
char calcBCC2 (char* buffer, int length);
void makeBufferStuffed (char* bufferStuffed, char* buffer, int length);
int getNumOfEscapeCharactersRequired (char* buffer, int length);

void configLinkLayer(char* port, int baudRate, unsigned int timeout, unsigned int numTransmissions) {
    strcpy(linkLayer->port, port);
    linkLayer->baudRate = baudRate;
    linkLayer->sequenceNumber = 0;
    linkLayer->timeout = timeout;
    linkLayer->numTransmissions = numTransmissions;
}

int llopen (int porta, int flagMode) {

    struct termios oldtio, newtio;

    int fd = open(linkLayer->port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) { perror(linkLayer->port); exit(-1); }

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

    (void) signal(SIGALRM, atende);

    if (flagMode == TRANSMITTER) {
        if (tryConnectInModeTransmitter(fd) == -1)
            return -1;
    }
    else if (flagMode == RECEIVER) {

    }
    else {
        return -1;
    }

    return fd;

}

void atende()                   // atende alarme
{
	printf("Try number: %d\n", conta);
	flag = 1;
	conta++;
}

int tryConnectInModeTransmitter(int fd) {
    while (conta <= linkLayer->numTransmissions && STOP != TRUE) {
        if (flag) {
            sendSET(fd);
            alarm(linkLayer->timeout);
            flag = 0;
            readUA(fd);
        }
    }
    if (STOP == FALSE) {
        return -1;
    } else {
        flag = 1;
        conta = 1;
        return 0;
    }
}

void sendSET(int fd) {
    unsigned char SET[5];
	SET[0] = F;
	SET[1] = A;
	SET[2] = C_SET;
	SET[3] = SET[1] ^ SET[2];
	SET[4] = F;
	int w = write(fd, SET, 5);
	printf("SEND SET: %d bytes written\n", w);
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
                    printf("UA message successful receive!\n");
                }
                break;
            }
    }
}

int llwrite (int fd, char * buffer, int length) {

    int numEscapeCharRequired = getNumOfEscapeCharactersRequired(buffer, length);
    char* bufferStuffed = (char*) malloc(length + numEscapeCharRequired);
    makeBufferStuffed(bufferStuffed, buffer, length);

    int sizeOfHeaderAndTrailer = 6; // F, A, C, BCC1, BCC2 e F
    int frameLength = length + sizeOfHeaderAndTrailer + numEscapeCharRequired;
    char* frame = (char*) malloc(frameLength);
    frame[0] = F;
    frame[1] = A;
    frame[2] = (linkLayer->sequenceNumber << 6); // Campo de Controlo = 0 S 0 0 0 0 0 0 , em que S = N(s) = 0 ou 1 (ver slide 7 do guião)
    frame[3] = frame[1] ^ frame[2]; // BCC1
    char BCC2 = calcBCC2(buffer, length);
    frame[frameLength-2] = BCC2;
    frame[frameLength-1] = F;






    return -1;
}

int getNumOfEscapeCharactersRequired (char* buffer, int length) {
    int numEscapeChar = 0;
    int i;
    for (i = 0; i < length; i++) {
        if (buffer[i] == F || buffer[i] == ESCAPE) {
            numEscapeChar++;
        }
    }
    return numEscapeChar;
}

void makeBufferStuffed (char* bufferStuffed, char* buffer, int length) {
    int bufIndex, bufStuffedIndex = 0;
    for (bufIndex = 0; bufIndex < length; bufIndex++) {
        if (buffer[bufIndex] == F || buffer[bufIndex] == ESCAPE) {
            bufferStuffed[bufStuffedIndex++] = ESCAPE;
            bufferStuffed[bufStuffedIndex++] = buffer[bufIndex] ^ 0x20; // ver slide 13
        } else {
            bufferStuffed[bufStuffedIndex++] = buffer[bufIndex];
        }
    }
}

char calcBCC2 (char* buffer, int length) {
    char BCC2 = 0;
    int i;
    for (i = 0; i < length; i++) {
        BCC2 ^= buffer[i];
    }
    return BCC2;
}


int llread (int fd, char * buffer) {
    return -1;
}

int llclose (int fd) {
    return -1;
}
