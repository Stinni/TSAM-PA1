// TFTP Server - Programming Assignment 1 for Computer Networking
// University of Reykjavík, autumn 2017
// Students: Hreiðar Ólafur Arnarsson, Kristinn Heiðar Freysteinsson & Maciej Sierzputowski
// Usernames: hreidara14, kristinnf13 & maciej15

// Constants:
#define MAX_MESSAGE_LENGTH   512
#define MAX_PACKET_SIZE      516
#define MAX_FILE_NAME_LENGTH 255
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
#include <arpa/inet.h>
//#include <sys/select.h>
//#include <time.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h> /* PATH_MAX */

// Global variables - These variables have to be accessible in nearly all functions.
// To not have to send them into each function, they're declared here instead.
socklen_t len;
FILE *fp = NULL;
char *toBeSent;
int toBeSentLength = 0;
int server_busy = 0;
int transfer_complete = 0;
unsigned int block_nr = 1;
int timeouts = 0;

void RRequest(int sockfd, struct sockaddr_in client, char *msg, char *folder_path, char *base_path)
{

	char file_name[MAX_FILE_NAME_LENGTH], tmp[PATH_MAX];
	char *file_path;

	strcpy(tmp, folder_path);
	strcpy(file_name, &msg[2]);
	strcat(tmp, file_name);

	printf("File \"%s\" requested from %s:%d\n", file_name, (char *)inet_ntoa(client.sin_addr), ntohs(client.sin_port));

	if(server_busy) {
		printf("Client sent RRQ request while work in progress\n");
		char errPacket[38];
		errPacket[0] = 0; errPacket[1] = OP_ERR; errPacket[2] = 0; errPacket[3] = ERR_NOT;
		strcpy(&errPacket[4], "Work in progress, try again later");
		sendto(sockfd, errPacket, sizeof(errPacket), 0, (struct sockaddr *) &client, len);
		return;
	}

	file_path = realpath(tmp, NULL);

	if(file_path == NULL) {
		// An error package is sent to the client if the file requested doesn't exists
		printf("File \"%s\" doesn't exist\n", file_name);
		char errPacket[19];
		errPacket[0] = 0; errPacket[1] = OP_ERR; errPacket[2] = 0; errPacket[3] = ERR_FILE;
		strcpy(&errPacket[4], "File not found");
		sendto(sockfd, errPacket, sizeof(errPacket), 0, (struct sockaddr *) &client, len);
	}
	else {
		if(strncmp(base_path, file_path, strlen(base_path)) != 0)
		{
			printf("NOTE!!! Access violation, the client tried to access a file outside the \"%s\" folder!\n", folder_path);
			char errPacket[21];
			errPacket[0] = 0; errPacket[1] = OP_ERR; errPacket[2] = 0; errPacket[3] = ERR_ACC;
			strcpy(&errPacket[4], "Access violation");
			sendto(sockfd, errPacket, sizeof(errPacket), 0, (struct sockaddr *) &client, len);
		}
		else {
			fp = fopen(file_path, "r");
			if(fp == NULL) {
				// By this stage the file should exist and fp shouldn't be NULL but better check, just in case...
				printf("File \"%s\" couldn't be opened\n", file_path);
				char errPacket[19];
				errPacket[0] = 0; errPacket[1] = OP_ERR; errPacket[2] = 0; errPacket[3] = ERR_FILE;
				strcpy(&errPacket[4], "File not found");
				sendto(sockfd, errPacket, sizeof(errPacket), 0, (struct sockaddr *) &client, len);
			}
			else {
				printf("File \"%s\" was opened\n", file_path);
				server_busy++;
				char tmpMsg[MAX_MESSAGE_LENGTH];
				int tmpMsgLength = fread(&tmpMsg, 1, MAX_MESSAGE_LENGTH, fp); // up to 512 bytes of the file is copied into the package.
				//printf("tmpMsg: %s\n", tmpMsg);
				if(tmpMsgLength < MAX_MESSAGE_LENGTH) {
					toBeSentLength = tmpMsgLength + 4;
					toBeSent = realloc(toBeSent, toBeSentLength);
					toBeSent[0] = 0; toBeSent[1] = OP_DATA; toBeSent[2] = block_nr >> 8; toBeSent[3] = block_nr & 0xff;
					memcpy(&toBeSent[4], tmpMsg, tmpMsgLength);
					transfer_complete++;
				}
				else {
					toBeSentLength = MAX_PACKET_SIZE;
					toBeSent = realloc(toBeSent, toBeSentLength);
					toBeSent[0] = 0; toBeSent[1] = OP_DATA; toBeSent[2] = block_nr >> 8; toBeSent[3] = block_nr & 0xff;
					memcpy(&toBeSent[4], tmpMsg, tmpMsgLength);
					transfer_complete = 0;
				}
				sendto(sockfd, toBeSent, toBeSentLength, 0, (struct sockaddr *) &client, len);
			}
		}
	}
}

void WRequest(int sockfd, struct sockaddr_in client)
{
	printf("A WRQ request recieved from \"%s:%d\"\n", (char *)inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	char errorPacket[45];
	errorPacket[0] = 0; errorPacket[1] = OP_ERR; errorPacket[2] = 0; errorPacket[3] = ERR_IOP;
	strcpy(&errorPacket[4], "Uploading is not allowed on this server!");
	sendto(sockfd, errorPacket, sizeof(errorPacket), 0, (struct sockaddr *) &client, len);
}

void AckReceived(int sockfd, struct sockaddr_in client, char *message)
{
	unsigned int recv_block_nr = (unsigned char)message[2] << 8 | (unsigned char)message[3];

	if(server_busy && !transfer_complete && block_nr == recv_block_nr) {
		timeouts = 0;
		block_nr++;
		char tmpMsg[MAX_MESSAGE_LENGTH];
		int tmpMsgLength = fread(&tmpMsg, 1, MAX_MESSAGE_LENGTH, fp);
		if(tmpMsgLength < MAX_MESSAGE_LENGTH) {
			toBeSentLength = tmpMsgLength + 4;
			toBeSent = realloc(toBeSent, toBeSentLength);
			toBeSent[0] = 0; toBeSent[1] = OP_DATA; toBeSent[2] = block_nr >> 8; toBeSent[3] = block_nr & 0xff;
			memcpy(&toBeSent[4], tmpMsg, tmpMsgLength);
			transfer_complete++;
		}
		else {
			toBeSentLength = MAX_PACKET_SIZE;
			toBeSent = realloc(toBeSent, toBeSentLength);
			toBeSent[0] = 0; toBeSent[1] = OP_DATA; toBeSent[2] = block_nr >> 8; toBeSent[3] = block_nr & 0xff;
			memcpy(&toBeSent[4], tmpMsg, tmpMsgLength);
			transfer_complete = 0;
		}
		sendto(sockfd, toBeSent, toBeSentLength, 0, (struct sockaddr *) &client, len);
	}
	else if(server_busy && transfer_complete && block_nr == recv_block_nr)
	{
		printf("File transfer finished!\n");
		if(fp != NULL)
		{
			fclose(fp);
		}
		timeouts = 0;
		server_busy = 0;
		transfer_complete = 0;
		block_nr = 1;
	}
	else
	{
		printf("Recieved an ACK packet and no operation is ongoing\n");
	}
}

void ErrorReceived(struct sockaddr_in client, char *message)
{
	printf("An error message recieved from \"%s:%d\"", (char *)inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	/*char errcode = message[4];
	switch(errcode)
	{
		case ERR_NOT :
			printf("Not defined, see error message (if any).");
			break;
		case ERR_FILE :
			printf("File not found.");
			break;
		case ERR_ACC :
			printf("Access violation.");
			break;
		case ERR_DISK :
			printf("Disk full or allocation exceeded.");
			break;
		case ERR_IOP :
			printf("Illegal TFTP operation.");
			break;
		case ERR_UID :
			printf("Unknown transfer ID.");
			break;
		case ERR_FEX :
			printf("File already exists.");
			break;
		case ERR_USR :
			printf("No such user.");
			break;
	}*/
	if(server_busy) // We terminate any transfer upon recieveing an error message
	{
		transfer_complete = 0;
		block_nr = 1;
		server_busy = 0;
		timeouts = 0;
		if(fp != NULL)
		{
			fclose(fp);
		}
		printf("File transfer aborted\n");
	}
}

int main(int argc, char *argv[])
{

	int socket_addr;
	char *folder_path, *base_path;

	if(argc >= 3) {
		socket_addr = atoi(argv[1]);

		int tmp = strlen(argv[2]) + 2;
		folder_path = malloc(tmp);
		strcpy(folder_path, argv[2]);
		folder_path[tmp-2] = '/';
		folder_path[tmp-1] = '\0';
		base_path = realpath(folder_path, NULL);

		if(base_path == NULL) {
			free(folder_path);
			printf("Invalid directory provided!\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		printf("Format expected is .src/tftp <port_number> <directory>\n");
		exit(EXIT_FAILURE);
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

	printf("Server listening!\n");
	fflush(stdout);

	for (;;) {
		ssize_t n = 0;
		memset(&message, 0, sizeof(message));

		// Receive up to one byte less than declared, because it will
		// be NUL-terminated later.
		len = (socklen_t) sizeof(client);
		n = recvfrom(sockfd, message, sizeof(message) - 1,
							 0, (struct sockaddr *) &client, &len);

		if (n >= 0) {
			message[n] = '\0';
			char opcode = message[1];
			switch(opcode) {
				case OP_RRQ :
					RRequest(sockfd, client, message, folder_path, base_path);
					break;
				case OP_WRQ :
					WRequest(sockfd, client);
					break;
				case OP_DATA :
					printf("Recieved a data packet from \"%s:%d\"\n", (char *)inet_ntoa(client.sin_addr), ntohs(client.sin_port));
					break;
				case OP_ACK :
					AckReceived(sockfd, client, message);
					break;
				case OP_ERR :
					ErrorReceived(client, message);
					break;
				default:
					printf("Opcode unrecognised!\n");
			}
			fflush(stdout);
		}
		else
		{
			if((errno == EAGAIN || errno == EWOULDBLOCK))
			{
				if(server_busy && timeouts < MAX_TIMEOUTS)
				{
					timeouts++;
					printf("Timeout nr. %d - Resending last package\n", timeouts);
					sendto(sockfd, toBeSent, toBeSentLength, 0, (struct sockaddr *) &client, len);
				}
				else if(server_busy) // max timeouts reached, we reset everything and stop resending.
				{
					printf("Max number of timeouts has been reached. Aborting file transfer\n");
					transfer_complete = 0;
					block_nr = 1;
					server_busy = 0;
					timeouts = 0;
					if(fp != NULL)
					{
						fclose(fp);
					}
				}
			}
			else
			{
				printf("Some kind of an error happened...\n");
				printf("Goodbye!\n");
				exit(EXIT_FAILURE);
			}
			fflush(stdout);
		}
	}
}
