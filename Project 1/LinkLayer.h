#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define F 0x7e
#define A_RECEIVER 0x01
#define A_SENDER 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_RR 0x05
#define C_REJ 0x01
#define C_DISC 0x0B
#define ESCAPE 0x7D

#define TRANSMITTER 0
#define RECEIVER 1
#define MAX_FRAME_SIZE 1024

#define PASS_IN_STATE_MACHINE 5

typedef struct LinkLayers {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate;
    unsigned int sequenceNumber; /* Número de sequência da trama: 0, 1*/
    unsigned int timeout;
    unsigned int numTransmissions; /* Número de tentativas em caso de falha*/
    char frame[MAX_FRAME_SIZE]; /*Trama*/
} LinkLayer;

LinkLayer linkLayer;

void configLinkLayer(char* port, int baudRate, unsigned int timeout, unsigned int numTransmissions);

/*
argumentos
– porta: 0 ou 1, /dev/ttySx, x = 0, 1
– flagMode: TRANSMITTER / RECEIVER
retorno
– identificador da ligação de dados (file descriptor)
– valor negativo em caso de erro
*/
int llopen (int porta, int flagMode);

/*
argumentos
– fd: identificador da ligação de dados
– buffer: array de caracteres a transmitir
– length: comprimento do array de caracteres
retorno
– número de caracteres escritos
– valor negativo em caso de erro
*/
int llwrite (int fd, char * buffer, int length);

/*
argumentos
– fd: identificador da ligação de dados
– buffer: array de caracteres recebidos
retorno
– comprimento do array (número de caracteres lidos)
– valor negativo em caso de erro
*/
int llread (int fd, char * buffer);

/*
argumentos
– fd: identificador da ligação de dados
retorno
– valor positivo em caso de sucesso
– valor negativo em caso de erro
*/
int llclose (int fd);
