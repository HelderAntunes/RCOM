#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_SIZE 256

void parse_url (const char * url, char * user, char * password, char * host, char * url_path);
int get_ip (char *host, char *ip);
int make_socket (char *ip, int port);
int login (int socket, char *user, char *password);
int get_data_port (int command_socket);
int download_file(int command_socket, int data_socket, char *url_path);

int main (int argc, char *argv[]) {

	if (argc == 2) {

        char * url = argv[1];
        char user[MAX_SIZE];
        char password[MAX_SIZE];
        char host[MAX_SIZE];
        char url_path[MAX_SIZE];
        parse_url(url, user, password, host, url_path);
        printf("url parsed: %s %s %s %s\n", user, password, host, url_path);

        char ip[MAX_SIZE];
        get_ip(host, ip);
        printf("ip: %s\n", ip);

        int command_socket = make_socket(ip, 21); // 21 is a convention
        if (login(command_socket, user, password) != 0) {
            printf("error in login()\n");
            return -1;
        }
        printf("User %s logged in\n", user);

        int data_port = get_data_port(command_socket);
        printf("'pasv' command sended\n");

        int data_socket = make_socket(ip, data_port);

        if (download_file(command_socket, data_socket, url_path) == 0) {
            printf("file downloaded\n");
        }

        close(command_socket);
		close(data_socket);

	} else {
        printf("use: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }

	return 0;
}

/*
 * Read user, password, host and url-path from url of type:
 * 'ftp://[<user>:<password>@]<host>/<url-path>''.
 * Examples:
 * - ftp://user:password@host.com/urlpath
 * - ftp://host.com/urlpath
 */
void parse_url (const char * url, char * user, char * password, char * host, char * url_path) {

    char * aux, * aux2;
	int len, userAndPass_required = 0;

	aux = strchr(url, '/') + 2;
	aux2 = strchr(aux, ':');

	if (aux2 != 0) { // contains user and password
        userAndPass_required = 1;
		aux2--; // end of user

		// user
		len = aux2 - aux + 1;
		strncpy(user, aux, len);
		user[len] = '\0';

		// password
		aux2 += 2; // beginning of password
		aux = strchr(aux2, '@') - 1; // end of password
		len = aux - aux2 + 1;
		strncpy(password, aux2, len);
		password[len] = '\0';

		aux += 2; // beginning of host

	}

	// host
	aux2 = strchr(aux, '/') - 1; // end of host
	len = aux2 - aux + 1;
	strncpy(host, aux, len);
	host[len] = '\0';

	// url_path
	aux2 += 2; // beginning of url_path
	strcpy(url_path, aux2);

    // How to Use Anonymous FTP -> https://tools.ietf.org/html/rfc1635
    if (!userAndPass_required) {
        strcpy(user, "anonymous");
        strcpy(password, "guest");
    }

}

/**
 * Given an host name, return the coresponding ip.
 */
int get_ip (char *host, char *ip) {
	struct hostent *h;

	if ((h = gethostbyname(host)) == NULL ) {
		herror("gethostbyname");
		exit(1);
	}

	strcpy(ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

	return 0;
}

/**
 * Make a socket that connects to ip of server through the port 'port'.
 */
int make_socket (char *ip, int port) {
	int sockfd, len;
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		exit(0);
	}

	/*connect to the server*/
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("connect()");
		exit(0);
	}

	return sockfd;
}

/**
 * Login to a ftp server.
 */
int login (int socket, char *user, char *password) {
	char buf[MAX_SIZE];
	int len = 0;

	strcpy(buf, "USER ");
	strcat(buf, user);
	strcat(buf, "\n");
	write(socket, buf, strlen(buf));
	bzero(buf, sizeof(buf));
	len = read(socket, buf, MAX_SIZE);

	bzero(buf, sizeof(buf));
	strcpy(buf, "PASS ");
	strcat(buf, password);
	strcat(buf, "\n");
	write(socket, buf, strlen(buf));
	bzero(buf, sizeof(buf));
	len = read(socket, buf, MAX_SIZE);
	buf[len] = '\0';

	if (strncmp(buf, "230", 3) != 0)
		return -1;

	return 0;
}

/*
 * Get data port of data channel by sending a 'pasv' command
 * to the server through command channel (Passive mode).
 */
int get_data_port (int command_socket) {
	char buf[MAX_SIZE];
	char num1[MAX_SIZE];
	char num2[MAX_SIZE];
	int n1, n2;

	char *aux, *aux2;
	int len = 0;

	strcpy(buf, "pasv\n");
	write(command_socket, buf, strlen(buf));
	bzero(buf, sizeof(buf));

	len = read(command_socket, buf, MAX_SIZE);
	buf[len] = '\0';
	if (strncmp(buf, "227", 3) != 0) {
        printf("error in pasv command\n");
        return -1;
    }

	aux = strchr(buf, '(');
	aux = strchr(aux, ',') + 1;
	aux = strchr(aux, ',') + 1;
	aux = strchr(aux, ',') + 1;
	aux = strchr(aux, ',') + 1;
	aux2 = strchr(aux, ',') - 1;

	len = aux2 - aux + 1;
	strncpy(num1, aux, len);
	num1[len] = '\0';

	aux2 += 2;
	aux = strchr(aux2, ')') - 1;

	len = aux - aux2 + 1;
	strncpy(num2, aux2, len);
	num2[len] = '\0';

	n1 = atoi(num1);
	n2 = atoi(num2);

	return (n1 * 256 + n2);
}

/*
 * Download the file specified in 'url_path':
 * After send th 'retr' command to the command channel, read the file
 * from data channel.
 */
int download_file(int command_socket, int data_socket, char *url_path) {
	char buf[MAX_SIZE];
	char fileName[MAX_SIZE];
	char * temp;
	strcpy(fileName, url_path);
	int len, filefd, r;

	strcpy(buf, "retr ");
	strcat(buf, url_path);
	strcat(buf, "\n");
	write(command_socket, buf, strlen(buf));

    bzero(buf, sizeof(buf));
	len = read(command_socket, buf, MAX_SIZE);
	buf[len] = '\0';
	if (strncmp(buf, "150", 3) != 0) {
        printf("error in 'retr' command\n");
        return -1;
    }
	bzero(buf, sizeof(buf));

	while ((temp = strchr(fileName, '/')))
		strcpy(fileName, temp + 1);
	filefd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0777);

	while ((r = read(data_socket, buf, MAX_SIZE)) >= 1) {
		write(filefd, buf, r);
	}
	close(filefd);

	return 0;
}
