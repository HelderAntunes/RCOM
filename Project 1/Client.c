#include "Client.h"
#include <stdio.h>
#include <termios.h>

int chooseBaudrate(){
	int choice;

	printf("Baud Rate options (0 for default)\n");
	printf("300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200\n");

	do{
		printf("Enter Baud Rate: ");
		scanf("%d", &choice);

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

	scanf("%d", &choice);

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

	scanf("%d", &choice);

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

	scanf("%d", &choice);

	if(choice > 1){
		return choice;
	}
	else{
		printf("Invalid value. Will be used default time out: %d\n", DEFAULT_TIMEOUT);
		return DEFAULT_TIMEOUT;
	}
}