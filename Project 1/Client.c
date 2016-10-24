#include "Client.h"
#include <stdio.h>
#include <termios.h>
#include <string.h>

void clearStdin(){
	int c;
	while((c = getchar()) != '\n' && c != EOF){}
}

void choosePort(char* port){
	int choice;

	printf("Port Options:\n\t0 - /dev/ttyS0\n\t1 - /dev/ttyS1\n");
	do{
		printf("Enter Port : ");
		if(scanf("%d", &choice) == 0){
			clearStdin();
			continue;
		}

		switch(choice){
			case 0:
				strcpy(port , "/dev/ttyS0");
				return;
			case 1:
				strcpy(port , "/dev/ttyS1");
				return;
			default:
				printf("Invalid port. Please choose again\n");
		}

	}while(1);
}

int chooseBaudrate(){
	int choice;

	printf("Baud Rate options (0 for default)\n");
	printf("300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200\n");

	do{
		printf("Enter Baud Rate: ");
		if(scanf("%d", &choice) == 0){
			clearStdin();
			continue;
		}

		switch(choice){
			case 0:
				return BAUDRATE_DEFAULT;
			case 300:
				return BAUDRATE_300;
			case 600:
				return BAUDRATE_600;
			case 1200:
				return BAUDRATE_1200;
			case 2400:
				return BAUDRATE_2400;
			case 4800:
				return BAUDRATE_4800;
			case 9600:
				return BAUDRATE_9600;
			case 19200:
				return BAUDRATE_19200;
			case 38400:
				return BAUDRATE_38400;
			case 57600:
				return BAUDRATE_57600;
			case 115200:
				return BAUDRATE_115200;
			default:
				printf("Invalid Baud Rate. PLease choose again\n");
		}

	}while(1);
}

int chooseMaxPktSize(){
	int choice;

	printf("Enter packets maximum size: ");

	if(scanf("%d", &choice) == 0){
		printf("Invalid value. Will be used default packet size: %d\n", MAX_SIZE);
		clearStdin();
		return MAX_SIZE;
	}

	if(choice >= MIN_SIZE && choice <= MAX_SIZE){
		return choice;
	}
	else{
		printf("Invalid value. Will be used default packet size: %d\n", MAX_SIZE);
		return MAX_SIZE;
	}
}

int chooseMaxRetries(){
	int choice;

	printf("Enter maximum number of retries: ");

	if(scanf("%d", &choice) == 0){
		clearStdin();
		printf("Invalid value. Will be used default retransmission number: %d\n", DEFAULT_RETRIES);
		return DEFAULT_RETRIES;
	}

	if(choice > 1){
		return choice;
	}
	else{
		printf("Invalid value. Will be used default retransmission number: %d\n", DEFAULT_RETRIES);
		return DEFAULT_RETRIES;
	}
}

int chooseTimeOut(){
	int choice;

	printf("Enter time out in seconds: ");

	if(scanf("%d", &choice) == 0){
		clearStdin();
		printf("Invalid value. Will be used default time out: %d\n", DEFAULT_TIMEOUT);
		return DEFAULT_TIMEOUT;
	}

	if(choice > 1){
		return choice;
	}
	else{
		printf("Invalid value. Will be used default time out: %d\n", DEFAULT_TIMEOUT);
		return DEFAULT_TIMEOUT;
	}
}
