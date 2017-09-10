// TFTP Server - Programming Assignment 1 for Computer Networking
// University of Reykjavík, autumn 2017
// Students: Hreiðar Ólafur Arnarsson, Kristinn Heiðar Freysteinsson & Maciej Sierzputowski
// Usernames: hreidara14, kristinnf13 & maciej15

// Constants:
#define MAX_MESSAGE_LENGTH   512
#define MAX_PACKET_SIZE      516
#define MAX_FILE_PATH_LENGTH 255
#define MAX_TIMEOUTS           5

// OpCodes defined for readability
#define OP_RRQ  1 // Read request
#define OP_WRQ  2 // Write request
#define OP_DATA 3 // Data
#define OP_ACK  4 // Acknowledgment
#define OP_ERR  5 // Error

// Error Codes
#define ERR_NOT  0 // Not defined, see error message (if any).
#define ERR_FILE 1 // File not found.
#define ERR_ACC  2 // Access violation.
#define ERR_DISK 3 // Disk full or allocation exceeded.
#define ERR_IOP  4 // Illegal TFTP operation.
#define ERR_UID  5 // Unknown transfer ID.
#define ERR_FEX  6 // File already exists.
#define ERR_USR  7 // No such user.

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

//void RRequest(int sockfd, struct client, int message)
void RRequest()
{
  //message[0] + [1] = opcode
  //messagge[2] = filename

}

void WRequest(int sockfd, struct sockaddr_in *client, socklen_t *len)
{
  printf("WRQ request not allowed on server! \n");
  char errorPacket[45];
  errorPacket[0] = 0; errorPacket[1] = OP_ERR; errorPacket[2] = 0; errorPacket[3] = ERR_IOP;
  strcpy(&errorPacket[4], "Uploading is not allowed on this server!");
  sendto(sockfd, errorPacket, sizeof(errorPacket), 0, (struct sockaddr *) client, (size_t) len);
/*
    printf("Client sent WRQ request. Not supported!\n");
    char errPacket[27];
    errPacket[0] = 0; errPacket[1] = OP_ERR; errPacket[2] = 0; errPacket[3] = ERR_ACC;
    strcpy(&errPacket[4], "Uploading not supported");
    sendto(sockfd, errPacket, sizeof(errPacket), 0, (struct sockaddr *) &client, len);
*/
}

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
		ssize_t n = 0;
		memset(&message, 0, sizeof(message));

		// Receive up to one byte less than declared, because it will
		// be NUL-terminated later.
		socklen_t len = (socklen_t) sizeof(client);
		n = recvfrom(sockfd, message, sizeof(message) - 1,
							 0, (struct sockaddr *) &client, &len);
      
      char opcode = message[1];
      switch(opcode) {
        case '1' :
        	printf("Read request \n");
        	RRequest(sockfd, client, message);
        	break;
        case '2' :
        	printf("Write request \n");
        	WRequest(sockfd, &client, &len);
        	break;
        case '3' :
          	printf("Data \n");
        	break;
        case '4' :
        	printf("Acknowledgment \n");
        	break;
        case '5' :
        	printf("Error \n");
        	break;
      }
/*
 OP_RRQ  1 // Read request
 OP_WRQ  2 // Write request
 OP_DATA 3 // Data
 OP_ACK  4 // Acknowledgment
 OP_ERR  5 // Error
*/          
          
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

