
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "LinkLayer.h"

typedef struct ApplicationLayers {
    int fileDescriptor;
    int mode; /* TRANSMITTER | RECEIVER */
} ApplicationLayer;


int main(int argc, char** argv)
{
    printf("fdfsf\n");
	
	char* port = "/dev/ttyS0";
	
	configLinkLayer(port, BAUDRATE, 3, 5);
	
	if(argv[1] == "T"){
		llopen(0, TRANSMITTER);
	}
	else
		llopen(0, RECEIVER);    

    return 0;
}
