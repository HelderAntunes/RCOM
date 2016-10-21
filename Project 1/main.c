#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "LinkLayer.h"
#include "ApplicationLayer.h"

int main(int argc, char** argv)
{
    printf("fdfsf\n");
	
	char* port = "/dev/ttyS0";
	
	configLinkLayer(port, BAUDRATE, 3, 5);
	
	if(argv[1][0] == 'T'){
		if (initAppLayer(port, TRANSMITTER, "./pinguim.gif", 3, 5, 200, 38000) == -1) {
			printf("Error on initAppLayer()\n");
			return -1;
		}
	}
	else{
		if (initAppLayer(port, RECEIVER, "./pinguim.gif", 3, 5, 200, 38000) == -1) {
			printf("Error on initAppLayer()\n");
			return -1;
		}
	}   

    return 0;
}
