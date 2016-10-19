
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
	
	if(argv[1][0] == 'T'){
		llopen(0, TRANSMITTER);
	}
	else{
		int fd = llopen(0, RECEIVER); 

		int j;
		
		for(j = 0; j < 5; j++){
			unsigned char buffer[100];
			int size = llread (fd, buffer); 
			int i;

			for(i = 0; i < size; i++){
				printf("buffer %02x\n", buffer[i]);
			}
		}
	}   

    return 0;
}
