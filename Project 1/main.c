#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "LinkLayer.h"
#include "ApplicationLayer.h"
#include "Client.h"

int main(int argc, char** argv)
{
    if(argc != 2){
		printf("Usage: %s T/R\n", argv[0]);
		return -1;
	}
	
	char port[12];

	choosePort(port);

	int baudRate = chooseBaudrate();
	int pktSize = chooseMaxPktSize();
	int maxRetries = chooseMaxRetries();
	int timeout = chooseTimeOut();
	
	if(argv[1][0] == 'T'){
		if (initAppLayer(port, TRANSMITTER, "./pinguim.gif", timeout, maxRetries, pktSize, baudRate) == -1) {
			printf("Error on initAppLayer()\n");
			return -1;
		}
	}
	else{
		char filePath[500];
		if (initAppLayer(port, RECEIVER, filePath, timeout, maxRetries, pktSize, baudRate) == -1) {
			printf("Error on initAppLayer()\n");
			return -1;
		}
	}   

    return 0;
}
