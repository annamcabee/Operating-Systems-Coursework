#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

/* Some constants that you can use */
#define ROT 13
#define BUFFER_SIZE 256

/* The port number you must setup the TCP socket on */
const unsigned short PORT = 2200;

/* Function prototype for the scramble string method */
static void scramble_string(char *str);


/* 
 * The main method. You must fill this out as described in the project description PDF
 */
int main() {

	/* listen for upcoming connections on port 2200
	* use INADDR_ANY
	* socket()
	* bind()
	* listen()
	*/ 

	int listenfd = 0, conn_socket = 0;
    struct sockaddr_in server_addr, client_addr; 
    char buffer[BUFFER_SIZE];
    socklen_t size;
	// socket()
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd==-1) {
		printf("socket error\n");
	}
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr,0,sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT); 
	// bind()
    if (bind(listenfd,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0){
        perror("bind failed. Error");
        int enable = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    		perror("setsockopt(SO_REUSEADDR) failed");
        	return 1;
       } else {
       		printf("reused address\n");
       }
    }
    printf("listenfd: %d\n", listenfd);
	// listen()
    if (listen(listenfd, ROT) == -1) {
		perror("Listen:\n");
	} 
    int num;
    while(1) {
		size = sizeof(struct sockaddr_in);
		printf("This is the size: %d\n");
		// accept()
		if ((conn_socket = accept(listenfd, (struct sockaddr *)&client_addr, &size))==-1 ) {
			perror("accept");
		}
		while(1) {
			// recv()
			if ((num = recv(conn_socket, buffer, BUFFER_SIZE-1, 0))== -1) {
				printf("recv error\n");
			} else if (num == 0) {
				printf("Connection closed\n");
				break;
			}
			buffer[num] = '\0';
			printf("The server recieved  this message: %s\n", buffer);
			scramble_string(buffer);
			// send()
			if ((send(conn_socket, buffer, strlen(buffer),0))== -1) {
				close(conn_socket);
				break;
			}
			printf("The server sent this message: %s\n", buffer);
		} 
		// close()
		close(conn_socket);
    } 
    // close()
	close(listenfd);
	return 0;
}
/*
 * This function takes in a NULL terminated ASCII, C string and scrambles it
 * using the popular ROT13 cipher. MODIFY AT YOUR OWN RISK
 *
 * @param str C-style string of maximum length 2^31 - 1
 */
static void scramble_string(char *str)
{
	int i;
	char t;
	for (i = 0; str[i]; i++) {
		t = str[i];
		if (t >= 'A' && t <= 'Z') {
			if ((t + ROT) <= 'Z') {
				str[i] += ROT;
			} else {
				str[i] -= ROT;
			}
		} else if (t >= 'a' && t <= 'z') {
			if ((t + ROT) <= 'z') {
				str[i] += ROT;
			} else {
				str[i] -= ROT;
			}
		}
	}
}
