#include "LinkLayer.h"

struct termios oldtio,newtio;
int mode;

volatile int STOP=FALSE;
int isToSendMessage=TRUE, conta=1;

void atende ();
int tryConnectInModeTransmitter (int fd);
int tryConnectInModeReceiver (int fd);
void sendSET (int fd);
int sendFrame(int fd, unsigned char a, unsigned char c);
char calcBCC2 (char* buffer, int length);
void makeBufferStuffed (char* bufferStuffed, char* buffer, int length);
int destuffFrame(unsigned char* frame, int frameSize, unsigned char* destuffedFrame);
int getNumOfEscapeCharactersRequired (char* buffer, int length);
int readSupervisonOrNonNumeratedFrame (int fd, char * frame);
int trySendFrame (int fd, char* frame, int frameLength);
int constructFrame (char* frame, char* buffer, int length);
void readConfirmation (int fd);
int receiveFrame(int fd, unsigned char* frame);
int tryDisconnectInModeTransmitter (int fd);
int tryDisconnectInModeReceiver (int fd);

void configLinkLayer(char* port, int baudRate, unsigned int timeout, unsigned int numTransmissions) {
    strcpy(linkLayer.port, port);
    linkLayer.baudRate = baudRate;
    linkLayer.sequenceNumber = 0;
    linkLayer.timeout = timeout;
    linkLayer.numTransmissions = numTransmissions;
}

int llopen (int porta, int flagMode) {
	srand(time(NULL));

	mode = flagMode;

	int fd = open(linkLayer.port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) {perror(linkLayer.port); exit(-1); }

    tcgetattr(fd,&oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = linkLayer.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    (void) signal(SIGALRM, atende);

    if (mode == TRANSMITTER) {
    	if (tryConnectInModeTransmitter(fd) == -1)
            return -1;
    }
    else if (mode == RECEIVER) {
    	if (tryConnectInModeReceiver(fd) == -1)
            return -1;
    }
    else {
        return -1;
    }

    return fd;

}

void atende() {
    printf("Try number: %d\n", conta);
    isToSendMessage = TRUE;
    conta++;
}

int tryConnectInModeTransmitter(int fd) {

    STOP = FALSE;
    conta = 1;
    isToSendMessage = TRUE;

    while (conta <= linkLayer.numTransmissions && !STOP) {
        if (isToSendMessage) {
            sendSET(fd);
            alarm(linkLayer.timeout);
            isToSendMessage = FALSE;
            char UA[255];
            if (readSupervisonOrNonNumeratedFrame(fd, UA) == PASS_IN_STATE_MACHINE){
                STOP = TRUE;
            }
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

int tryConnectInModeReceiver(int fd) {
	char frame[255];

	isToSendMessage = FALSE;

	if (readSupervisonOrNonNumeratedFrame(fd, frame) == PASS_IN_STATE_MACHINE){
		if(frame[2] == C_SET) //Received SET message
			sendFrame(fd, A_SENDER, C_UA); // Answer with UA message
	}
	else
		return -1;
}

int sendFrame(int fd, unsigned char a, unsigned char c){
	unsigned char frame[5];

	frame[0] = F;
	frame[1] = a;
	frame[2] = c;
	frame[3] = frame[1] ^ frame[2];
	frame[4] = F;

	int w = write(fd, frame, 5);
	//printf("sendFrame: %d bytes written\n", w);

	if(w != 5){
		printf("ERROR in sendFrame(): could not send frame\n");
		return -1;
	}

	return 0;
}

int llwrite (int fd, char * buffer, int length) {

    char frame[MAX_FRAME_SIZE];
    int frameLength = constructFrame(frame, buffer, length); // adiciona header, trailer e faz stuffing

    STOP = FALSE;
    isToSendMessage = 1;
    conta = 1;
    while(!STOP && conta <= linkLayer.numTransmissions){
		trySendFrame(fd, frame, frameLength);
    }

	if (STOP) {
		return length;
	} else {
    	return -1; // indica que o numero de transmissoes excedeu o limite.
	}
}

int constructFrame (char* frame, char* buffer, int length) {

    // stuffing buffer
    char BCC2 = calcBCC2(buffer, length);
    buffer[length] = BCC2;
    int numEscapeCharRequired = getNumOfEscapeCharactersRequired(buffer, length + 1);
    char* bufferStuffed = (char*) malloc(length + 1 + numEscapeCharRequired);
    makeBufferStuffed(bufferStuffed, buffer, length + 1);

    // calcula tamanho da frame
    int sizeOfHeaderAndTrailer = 6; // F, A, C, BCC1, BCC2 e F
    int frameLength = length + sizeOfHeaderAndTrailer + numEscapeCharRequired;

    // preenche frame
    frame[0] = F;
    frame[1] = A_SENDER;
    frame[2] = (linkLayer.sequenceNumber << 6); // Campo de Controlo = 0 S 0 0 0 0 0 0 , em que S = N(s) = 0 ou 1 (ver slide 7 do guião)
    frame[3] = frame[1] ^ frame[2]; // BCC1
    memcpy(&frame[4], bufferStuffed, length + 1 + numEscapeCharRequired);

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
        write(fd, frame, frameLength);
        alarm(linkLayer.timeout);
        isToSendMessage = FALSE;
		readConfirmation(fd);
    }
    return 0;
}

void readConfirmation (int fd) {
    unsigned char confirmation[255];

    if (readSupervisonOrNonNumeratedFrame(fd, confirmation) == PASS_IN_STATE_MACHINE) {

        switch ((confirmation[2] & 0x0F)) {
            case C_REJ:
                if((confirmation[2] >> 7) == linkLayer.sequenceNumber){
                    alarm(0); // cancela alarme
                    isToSendMessage = 1;
                }
                break;
            case C_RR:
                if((confirmation[2] >> 7) != linkLayer.sequenceNumber){
                    alarm(0);
                    linkLayer.sequenceNumber = (confirmation[2] >> 7);
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

            if(isToSendMessage == TRUE && r == -1){ // quando o alarme dispara isToSendMessage fica TRUE e sai desta função
            	return -1;
            }

            if (r == -1) {
                continue;// ignorar erro(E_AGAIN) devido a NON_BLOCK
            }
            else{
            	//printf("readSupervisonOrNonNumeratedFrame: %d 0x%02x\n", r, c);
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

int receiveFrame(int fd, unsigned char* frame){
	int i = 0;
	int state = 0;
	int receiving = TRUE;

	unsigned char c;

	while(receiving){
		int r = read(fd, &c, 1);
		if(r == -1)
			continue; // ignorar erro(E_AGAIN) devido a NON_BLOCK

		switch(state){
		case 0:
			if(c == F){
				frame[i] = c;
				i++;
				state++;
			}
			break;
		case 1:
			if(c == A_SENDER){
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
			if(c == frame[1] ^frame[2]){
				frame[i] = c;
				i++;
				state++;
			}
			else if(c == F){
				state = 1;
				i = 1;
			}
			else{
				state = 0;
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

int destuffFrame(unsigned char* frame, int frameSize, unsigned char* destuffedFrame){

	int i;
	int j = 0;
	for (i = 0; i < frameSize; i++, j++) {
		if (frame[i] == ESCAPE)
			destuffedFrame[j] = frame[++i] ^ 0x20;
		else
			destuffedFrame[j] = frame[i];
	}

	return j;
}


int llread (int fd, char * buffer) {
	unsigned char frame[MAX_FRAME_SIZE];
	unsigned char destuffedFrame[MAX_FRAME_SIZE];

	//Read frame
	int frame_size = receiveFrame(fd,frame);
	if(frame_size == -1 || frame_size < 5){
		return -1;
	}

	if(frame_size == 5){
		if(frame[2] == C_SET){ //Received SET frame
			//sendFrame(fd, A_SENDER, C_UA);
			return 0;
		}
	}

	//Destuff frame
	int destuffedSize = destuffFrame(frame, frame_size, destuffedFrame);

	int dataSize = destuffedSize - 6; //6 bytes are used in prefix and posfix

	//Simulate error
	if(rand() % 200 == 1){
		printf("ERROR Simulation\n");
		int errorByte = (rand() % dataSize) + 4;
		destuffedFrame[errorByte] = C_UA;
	}

	//Check errors
	unsigned char BCC2 = calcBCC2(&destuffedFrame[4], dataSize);

    if (destuffedFrame[destuffedSize-2] != BCC2) {
        printf("ERROR in llread(): BCC2 error\n");

        unsigned int  seq = destuffedFrame[2] >> 6;
        if (seq == linkLayer.sequenceNumber) { // nova trama

            //Send Reject
            if(seq == 0){
                unsigned char C_REJ0 = (0 << 7) | C_REJ;
                sendFrame(fd, A_SENDER, C_REJ0);
            }
            else if(seq == 1){
                unsigned char C_REJ1 = (1 << 7) | C_REJ;
                sendFrame(fd, A_SENDER, C_REJ1);
            }
        }
        else { // trama duplicada
            if(seq == 0){
                int C_RR1 = (1 << 7) | C_RR;
                sendFrame(fd, A_SENDER, C_RR1);
            }
            else if(seq == 1){
                int C_RR0 = (0 << 7) | C_RR;
                sendFrame(fd, A_SENDER, C_RR0);
            }
        }
    }
	else{
		//Get frame's sequence number
		unsigned int  seq = destuffedFrame[2] >> 6;

		//Send ReceiverReady

		if(seq == 0){
			int C_RR1 = (1 << 7) | C_RR;
			sendFrame(fd, A_SENDER, C_RR1);
		}
		else if(seq == 1){
			int C_RR0 = (0 << 7) | C_RR;
			sendFrame(fd, A_SENDER, C_RR0);
		}

		if(seq == linkLayer.sequenceNumber){
			//Update frame number to be received
			if(linkLayer.sequenceNumber == 1)
				linkLayer.sequenceNumber = 0;
			else
				linkLayer.sequenceNumber = 1;
			//Fill buffer with data
			memcpy(buffer,&destuffedFrame[4],dataSize);

			return dataSize;
		}
	}

	return 0;
}

int tryDisconnectInModeTransmitter (int fd){
	STOP = FALSE;
    conta = 1;
    isToSendMessage = TRUE;

    while (conta <= linkLayer.numTransmissions && !STOP) {
        if (isToSendMessage) {
            sendFrame(fd, A_SENDER, C_DISC);
            alarm(linkLayer.timeout);
            isToSendMessage = FALSE;
            char DISC[255];
            if (readSupervisonOrNonNumeratedFrame(fd, DISC) == PASS_IN_STATE_MACHINE){
                STOP = TRUE;
				sendFrame(fd, A_SENDER, C_UA);
            }
        }
    }

    if (STOP == TRUE) {
        return 0;
    } else {
        return -1;
    }
}

int tryDisconnectInModeReceiver (int fd){
	char frame[255];
	if (readSupervisonOrNonNumeratedFrame(fd, frame) == PASS_IN_STATE_MACHINE){
		if(frame[2] == C_DISC){ //Received DISC message
			sendFrame(fd, A_RECEIVER, C_DISC); // Answer with DISC message

			if (readSupervisonOrNonNumeratedFrame(fd, frame) == PASS_IN_STATE_MACHINE){ //Wait for UA
				if(frame[2] == C_UA)
					return 0;
			}
		}
	}
	else
		return -1;
}

int llclose (int fd) {

	//Comunicate disconnection
	if (mode == TRANSMITTER) {
    	if (tryDisconnectInModeTransmitter(fd) == -1)
            return -1;
    }
    else if (mode == RECEIVER) {
    	if (tryDisconnectInModeReceiver(fd) == -1)
            return -1;
    }
    else {
        return -1;
    }

	//Close File
	tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);

	return 0;
}
