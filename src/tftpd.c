// TFTP Server - Programming Assignment 1 for Computer Networking
// University of Reykjavík, autumn 2017
// Students: Hreiðar Ólafur Arnarsson, Kristinn Heiðar Freysteinsson & Maciej Sierzputowski
// Usernames: hreidara14, kristinnf13 & maciej15

// Constants:
#define MAX_MESSAGE_LENGTH   512

// Included libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
//#include <sys/select.h>
//#include <time.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
	int socket_addr;

	if(argc >= 3) {
		socket_addr = atoi(argv[1]);
	}
	else {
		printf("Format expected is .src/tftp <port_number> <directory>\n");
		exit(EXIT_SUCCESS);
	}

	int sockfd;
	struct sockaddr_in server, client;
	char message[MAX_MESSAGE_LENGTH];

	// Create and bind a UDP socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// Receives should timeout after 30 seconds.
	struct timeval timeout;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// Network functions need arguments in network byte order instead
	// of host byte order. The macros htonl, htons convert the
	// values.
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(socket_addr);
	bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

	for (;;) {
		// Receive up to one byte less than declared, because it will
		// be NUL-terminated later.
		socklen_t len = (socklen_t) sizeof(client);
		ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
							 0, (struct sockaddr *) &client, &len);
		if (n >= 0) {
			message[n] = '\0';
			printf("Received:\n");
			for(unsigned int i = 0; i < n; i++) printf("%hhx ", message[i]);
			printf("\n");
			fflush(stdout);

		} else {
			// Error or timeout. Check errno == EAGAIN or
			// errno == EWOULDBLOCK to check whether a timeout happened
		}
	}
}
