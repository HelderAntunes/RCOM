
typedef struct LinkLayers {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate;
    unsigned int sequenceNumber; /* Número de sequência da trama: 0, 1*/
    unsigned int timeout;
    unsigned int numTransmissions; /* Número de tentativas em caso de falha*/
    char frame[MAX_SIZE]; /*Trama*/
} LinkLayer;

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
