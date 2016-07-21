#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
// idk if i should include this
#include <arpa/inet.h>

/* The port number you must create a TCP socket at */
const unsigned short PORT = 2200;

/*
 * The main function where you will be performing the tasks described under the
 * client section of the project description PDF
 */
int main(int argc, char **argv)
{
	/* Making sure that the correct number of command line areguments are being passed through */
	if (argc < 3) {
		fprintf(stderr, "\n\nclient usage: ./client <server IP address> <string to send>\n");
		exit(-1);
	}

	/**
	* parse command line arguments
	* convert port to network byte order
	**/

	const char* addrIP = argv[1];
	char* buffer = argv[2];
	int my_socket=0;
	struct sockaddr_in server_address;

	/** connect to server on Port 2200
	* inet_aton(…)
	* socket(..)
	* connect(..)
	**/
	// socket()
	if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Failed to make socket\n");
		return 1;
	}
    memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family=AF_INET;
	server_address.sin_port=htons(PORT);
	//  inet_aton()
	if (inet_aton(addrIP, &server_address.sin_addr) <=0) {
		printf("Error in inet_aton\n");
	}
	
	// connect()
	if (connect(my_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
		perror("Error");
	}
	/**
	 * send and then wait to recieve
	 * send()
	 * recv()
	 **/
	int num;
	while(1) {
		// send
        if ((send(my_socket,buffer, strlen(buffer),0))== -1) {
                close(my_socket);
                return 1;
        } else {
				printf("The client sent this message: %s\n", buffer);
                num = recv(my_socket, buffer, sizeof(buffer),0);
                if (num <= 0) {
                        printf("Either Connection Closed or Error\n");
                        break;
                }
                buffer[num] = '\0';
                //print()
				printf("The client recieved  this message: %s\n", buffer);
           }
    }
    // close
    close(my_socket);
	return 0;
}









