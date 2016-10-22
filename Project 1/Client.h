#ifndef Client
#define Client

//Valores possíveis para o Baud Rate
#define BAUDRATE_DEFAULT B38400
#define BAUDRATE_300 B300
#define BAUDRATE_600 B600
#define BAUDRATE_1200 B1200
#define BAUDRATE_2400 B2400
#define BAUDRATE_4800 B4800
#define BAUDRATE_9600 B9600
#define BAUDRATE_19200 B19200
#define BAUDRATE_38400 B38400
#define BAUDRATE_57600 B57600
#define BAUDRATE_115200 B115200

//Valores por definição
#define MIN_SIZE 100
#define MAX_SIZE 10000
#define DEFAULT_RETRIES 3
#define DEFAULT_TIMEOUT 5

//Escolha do Baudrate
int chooseBaudrate();

//Escolha do tamanho máximo campo de informação das tramas I
int chooseMaxPktSize();

//Escolha do número máximo de retransmissões
int chooseMaxRetries();

//Escolha do intervalo de time-out
int chooseTimeOut();

#endif