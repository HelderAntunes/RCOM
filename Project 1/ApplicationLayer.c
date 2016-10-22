#include "LinkLayer.h"
#include "ApplicationLayer.h"
#include <stdio.h>
#include <string.h>

FILE * openFile(char * filePath);
int initAppInModeTransmitter(char* filePath);
int initAppInModeReceiver(char* filePath);
int sendData(char * filePath, int fileSize);
int receiveData(char * filePath, int fileSize);
int sendCtrlPkt(int ctrlField, char * filePath, int fileSize);
int receiveCtrlPkt(int ctrlField, int * fileSize, char * filePath);
int sendDataPkt(char * buffer, int bytesRead, int sequenceNumber);
int receiveDataPkt(unsigned char * buffer,int sequenceNumber);

int initAppLayer(char * port, int mode, char * filePath, int timeout, int retries, int pktSize, int baudrate){
	configLinkLayer(port, baudrate, timeout, retries);

	al.fd = llopen(0, mode);

	printf("Connection established\n");

	if(al.fd == -1){
		return -1;
	}

	al.mode = mode;
	al.pktSize = pktSize;

	if(mode == TRANSMITTER){
		initAppInModeTransmitter(filePath);
	}
	else if(mode == RECEIVER){
		initAppInModeReceiver(filePath);
	}

	if(llclose(al.fd) == -1){
		return -1;
	}

	printf("Disconnected\n");

	return 0;
}

int initAppInModeTransmitter(char* filePath){
	al.file = openFile(filePath);

	int fileSize;

	struct stat st;
	if (stat(filePath, &st) == 0)
		fileSize = st.st_size;
	else {
		printf("ERROR getting file size!\n");
		return -1;
	}

	if(sendData(filePath, fileSize) < 0) {
		fclose(al.file);
		return -1;
	}


	if (fclose(al.file) < 0) {
		perror("Line 56, initAppInModeTransmitter");
		return -1;
	}

	return 0;
}

FILE * openFile(char * filePath) {

	FILE * file;

	if(al.mode == TRANSMITTER)
		file = fopen(filePath, "rb"); //Read in binary mode
	else if(al.mode == RECEIVER)
		file = fopen(filePath, "wb");

	if(file == NULL) {
		perror("openFile()");
			return NULL;
	}

	return file;
}

int sendData(char * filePath, int fileSize){
	if (sendCtrlPkt(C_START, filePath, fileSize) < 0)
		return -1;

	int bytesRead = 0;
	int sequenceNumber = 0;

	char* buffer = malloc(al.pktSize * sizeof(char));

	while((bytesRead = fread(buffer, sizeof(char), al.pktSize, al.file)) > 0){
		if(sendDataPkt(buffer, bytesRead, sequenceNumber) < 0)
			return -1;

		sequenceNumber = (sequenceNumber + 1) % 255;
	}

	if (sendCtrlPkt(C_END, filePath, fileSize) < 0)
		return -1;

	printf("File sent!\n");

	return 0;
}

int sendCtrlPkt(int ctrlField, char * filePath, int fileSize){
	char sizeString[16];
	sprintf(sizeString, "%d", fileSize);

	int pktSize = 5 + strlen(sizeString) + strlen(filePath); //size of header + trailer is 5

	unsigned char ctrlPckg[pktSize];

	ctrlPckg[0] = ctrlField; // unsigned char corresponding to int

	ctrlPckg[1] = 0; // T: tamanho do ficheiro
	ctrlPckg[2] = strlen(sizeString); // L: tamanho em octetos


	int i, acumulator = 3;
	for(i = 0; i < strlen(sizeString); i++) {
		ctrlPckg[acumulator++] = sizeString[i];
	}

	ctrlPckg[acumulator++] = 1; // T: nome do ficheiro
	ctrlPckg[acumulator++] = strlen(filePath); // L: tamanho em octetos

	for(i = 0; i < strlen(filePath); i++) {
		ctrlPckg[acumulator++] = filePath[i];
	}

	if (llwrite(al.fd, ctrlPckg, pktSize) < 0) {
		perror("sendCtrlPkt()");
		return -1;
	}

	return 0;
}

int sendDataPkt(char * buffer, int bytesRead, int sequenceNumber){
	int size = bytesRead + 4;
	unsigned char dataPckg[size];

	dataPckg[0] = C_DATA; // C
	dataPckg[1] = (unsigned char) sequenceNumber; // N

	dataPckg[2] = bytesRead / 256; // L2
	dataPckg[3] = bytesRead % 256; // L1
	memcpy(&dataPckg[4], buffer, bytesRead);

	if (llwrite(al.fd, dataPckg, size) < 0) {
		printf("ERROR in sendDataPkt(): llwrite() function error!\n");
		return -1;
	}

	return 0;
}

int initAppInModeReceiver(char * filePath){
	int fileSize;

	if(receiveCtrlPkt(C_START, &fileSize, filePath) < 0)
		return -1;

	al.file = openFile(filePath);

	if(receiveData(filePath, fileSize) < 0)
		return -1;

	if (fclose(al.file) < 0) {
		perror("initAppInModeReceiver()");
		return -1;
	}

	if (receiveCtrlPkt(C_END, &fileSize, filePath) < 0)
		return -1;

	return 0;
}

int receiveData(char * filePath, int fileSize){


	int bytesRead;
	int bytesAcumulator = 0;
	int sequenceNumber = 0;
	unsigned char * buffer = malloc(al.pktSize * sizeof(char));

	while (bytesAcumulator < fileSize){
		bytesRead = receiveDataPkt(buffer, sequenceNumber);

		if(bytesRead < 0)
			return -1;

		bytesAcumulator += bytesRead;
		fwrite(buffer, sizeof(char), bytesRead, al.file);

		if(bytesRead > 0) 
			sequenceNumber = (sequenceNumber + 1) % 255;
	}

	printf("File received!\n");

	return 0;
}

int receiveCtrlPkt(int ctrlField, int * fileSize, char * filePath){
	unsigned char info[MAX_PKT_SIZE];

	if (llread(al.fd, info) < 0) {
		perror("ERROR in receiveCtrlPkt(): llread failed\n");
		return -1;
	}

	if (info[0] != ctrlField) {
		printf("ERROR in receiveCtrlPkt(): unexpected control field!\n");
		return -1;
	}

	if (info[1] != FILE_SIZE) {
		printf("ERROR in receiveCtrlPkt(): unexpected size param!\n");
		return -1;
	}

	int i;
	int fileSizeLength = (int)info[2];
	int	acumulator = 3;

	char fileSizeStr[MAX_PKT_SIZE];

	for(i = 0; i < fileSizeLength; i++) {
		fileSizeStr[i] = info[acumulator++];
	}

	fileSizeStr[fileSizeLength] = '\0';

	(*fileSize) = atoi(fileSizeStr);

	if(info[acumulator++] != FILE_NAME) {
		printf("ERROR in receiveCtrlPkt(): unexpected name param!\n");
		return -1;
	}

	int pathLength = info[acumulator++];

	char pathStr[MAX_PKT_SIZE];

	for(i = 0; i < pathLength; i++) {
		pathStr[i] = info[acumulator++];
	}

	pathStr[pathLength] = '\0';
	strcpy(filePath, pathStr);

	return 0;
}

int receiveDataPkt(unsigned char * buffer,int sequenceNumber){
	unsigned char info[MAX_PKT_SIZE];
	int bytesRead = llread(al.fd, info);

	if (bytesRead < 0) {
		printf("ERROR in receiveDataPkt(): llread() function error!\n");
		return -1;
	}

	if (bytesRead == 0)
		return 0;

	int C = (int) info[0];
	int N = (int) info[1];

	if (C != C_DATA) {
		printf("ERROR in receiveDataPkt(): control field it's different from C_DATA!\n");
		return -1;
	}

	if (N != sequenceNumber) {
		printf("ERROR in receiveDataPkt(): sequence number it's wrong! %d != %d\n", N, sequenceNumber);
		return -1;
	}

	int L2 = info[2];
	int L1 = info[3];
	int pktSize = 256 * L2 + L1;

	memcpy(buffer, &info[4], pktSize);

	return pktSize;
}
