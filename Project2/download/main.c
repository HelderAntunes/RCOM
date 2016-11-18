#include <stdio.h>
#include <string.h>

#define MAX_SIZE 256

void parse_url (const char * url, char * user, char * password, char * host, char * url_path);

int main (int argc, char *argv[]) {

	if (argc==2) {

        char * url = argv[1];
        char user[MAX_SIZE];
        char password[MAX_SIZE];
        char host[MAX_SIZE];
        char url_path[MAX_SIZE];
        parse_url(url, user, password, host, url_path);
        printf("%s %s %s %s\n", user, password, host, url_path);

	} else {
        printf("use: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }

	return 0;
}

/*
 * Read user, password, host and url-path from url of type: 'ftp://[<user>:<password>@]<host>/<url-path>''.
 * Examples:
 * - ftp://user:password@host.com/urlpath
 * - ftp://host.com/urlpath
 */
void parse_url (const char * url, char * user, char * password, char * host, char * url_path) {

    char * aux, * aux2;
	int len;

	aux = strchr(url, '/') + 2;
	aux2 = strchr(aux, ':');


	if (aux2 != 0) { // contains user and password
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

}
