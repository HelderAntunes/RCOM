#include "LinkLayer.h"

volatile int STOP=FALSE;

int isToSendMessage=TRUE, conta=1;

void atende ();
int tryConnectInModeTransmitter (int fd);
void sendSET (int fd);
char calcBCC2 (char* buffer, int length);
void makeBufferStuffed (char* bufferStuffed, char* buffer, int length);
int getNumOfEscapeCharactersRequired (char* buffer, int length);
int readSupervisonOrNonNumeratedFrame (int fd, char * frame);
int trySendFrame (int fd, char* frame, int frameLength);
int constructFrame (char* frame, char* buffer, int length);
void readConfirmation (int fd);

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
    isToSendMessage = TRUE;
    conta++;
}

int tryConnectInModeTransmitter(int fd) {

    STOP = FALSE;
    conta = 1;
    isToSendMessage = TRUE;

    while (conta <= linkLayer->numTransmissions && STOP != TRUE) {
        if (isToSendMessage) {
            sendSET(fd);
            alarm(linkLayer->timeout);
            isToSendMessage = 0;
            char UA[255];
            if (readSupervisonOrNonNumeratedFrame(fd, UA) == PASS_IN_STATE_MACHINE)
                STOP = TRUE;
        }
    }
    if (STOP == TRUE) {
        return 0;
    } else {
        return -1;
    }

}

void sendSET(int fd) {
    unsigned char SET[5];
    SET[0] = F;
    SET[1] = A_SENDER;
    SET[2] = C_SET;
    SET[3] = SET[1] ^ SET[2];
    SET[4] = F;
    int w = write(fd, SET, 5);
    printf("SEND SET: %d bytes written\n", w);
}

int llwrite (int fd, char * buffer, int length) {

    char* frame;
    int frameLength = constructFrame(frame, buffer, length);

    STOP = FALSE;
    isToSendMessage = 1;
    conta = 1;
    while(!STOP){
        if (trySendFrame(fd, frame, frameLength) == -1) {
            free(frame);
            return -1;
        }
    }

    free(frame);

    return -1;
}

int constructFrame (char* frame, char* buffer, int length) {

    // stuffing buffer
    int numEscapeCharRequired = getNumOfEscapeCharactersRequired(buffer, length);
    char* bufferStuffed = (char*) malloc(length + numEscapeCharRequired);
    makeBufferStuffed(bufferStuffed, buffer, length);

    // calcula tamanho da frame
    int sizeOfHeaderAndTrailer = 6; // F, A, C, BCC1, BCC2 e F
    int frameLength = length + sizeOfHeaderAndTrailer + numEscapeCharRequired;
    frame = (char*) malloc(frameLength);

    // preenche frame
    frame[0] = F;
    frame[1] = A_SENDER;
    frame[2] = (linkLayer->sequenceNumber << 6); // Campo de Controlo = 0 S 0 0 0 0 0 0 , em que S = N(s) = 0 ou 1 (ver slide 7 do guião)
    frame[3] = frame[1] ^ frame[2]; // BCC1
    memcpy(&frame[4], bufferStuffed, length + numEscapeCharRequired);
    char BCC2 = calcBCC2(buffer, length);
    frame[frameLength-2] = BCC2;
    frame[frameLength-1] = F;

    return frameLength;
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

int trySendFrame (int fd, char* frame, int frameLength) {

    if(isToSendMessage){
        if (conta > linkLayer->numTransmissions) {
            printf("Error: number of transmissions exceeded.\n");
            return -1;
        }
        write(fd, frame, frameLength);
        alarm(linkLayer->timeout);
    }

    readConfirmation(fd);
    return 0;
}

void readConfirmation (int fd) {
    char confirmation[255];

    if (readSupervisonOrNonNumeratedFrame(fd, confirmation) == PASS_IN_STATE_MACHINE) {

        switch ((confirmation[2] & 0x0F)) {
            case C_REJ:
                if((confirmation[2] >> 7) == linkLayer->sequenceNumber){
                    alarm(0); // cancela alarme
                    isToSendMessage = 1;
                }
                break;
            case C_RR:
                if((confirmation[2] >> 7) != linkLayer->sequenceNumber){
                    alarm(0);
                    linkLayer->sequenceNumber = (confirmation[2] >> 7);
                    STOP = TRUE;
                }
                break;
            default:
                break;
        }
    }
}

int readSupervisonOrNonNumeratedFrame (int fd, char * frame) {
    volatile int passInStateMachine = FALSE;

    int state = 0;

    while (!passInStateMachine) {
        char c;
        if (state < 5) {
            int r = read(fd, &c, 1);
            if (r == -1) {
                return -1;
            }
            else if (r == 0 && isToSendMessage == 1) {  // quando o alarme dispara isToSendMessage fica a 1
                return -1;
            }
        }

        switch (state) {
            case 0:
            if (c == F) {
                frame[state] = c;
                state++;
            }
            break;
            case 1:
            if (c == A_SENDER || c == A_RECEIVER) {
                frame[state] = c;
                state++;
            }
            else if (c != F) {
                state = 0;
            }
            break;
            case 2:
            if (c == C_SET || c == C_UA || (c & 0x0F) == C_RR || (c & 0x0F) == C_REJ || c == C_DISC) {
                frame[state++] = c;
            }
            else if (c == F) {
                state = 1;
            }
            else {
                state = 0;
            }
            break;
            case 3:
            if (c == (frame[1] ^ frame[2])) {
                frame[state++] = c;
            }
            else if (c == F) {
                state = 1;
            }
            else {
                state = 0;
            }
            break;
            case 4:
            if (c == F) {
                frame[state++] = c;
            }
            else {
                state = 0;
            }
            break;
            default:
            passInStateMachine = TRUE;
            state = PASS_IN_STATE_MACHINE;
            break;
        }
    }

    return state;
}


int llread (int fd, char * buffer) {
    return -1;
}

int llclose (int fd) {
    return -1;
}
