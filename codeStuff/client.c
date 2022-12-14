#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG 0			 // We can print debugging comments by choice
#define BUFFER_SIZE 4096 // This is the maximum size of the message(s) on the control channel

#include "globals.h"
#include "client_state.h"
#include "server_input.h"
#include "client_commands.h"

// Code template from "rn3_simple_client.c" by Rohail Asim

int main()
{
	int server_sd = socket(AF_INET, SOCK_STREAM, 0); // Socket to be used for control channel

	if (server_sd < 0)
	{
		perror("socket:");
		exit(-1);
	}

	// Address of server

	struct sockaddr_in server_addr;

	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONTROL_PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct State state = {"", "", -1, -1, 1, -1}; // The state tracks the client's information for the server

	if (!initializePWD(&state))
	{ // Initialize the present working directory as the location in the client folder
		perror("Could not initialize state");

		return 0;
	}

	// setsock for timeout

	struct timeval tv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (setsockopt(server_sd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0)
	{
		perror("setsockopt failed");

		return 0;
	}

	// Connecting

	if (connect(server_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect");
		exit(-1);
	}

	char buffer[BUFFER_SIZE];

	struct sockaddr_in client_addr;

	int client_size = sizeof(client_addr);

	getsockname(server_sd, &client_addr, &client_size); // We need information about the IP address and port of the client

	strncpy(state.ipaddr, inet_ntoa(client_addr.sin_addr), MAX_IPADDRSTR_SIZE);

	state.port = ntohs(client_addr.sin_port);

	state.server_sd = server_sd;

	//
	bzero(buffer, sizeof(buffer));
	read(server_sd, buffer, sizeof(buffer));
	if (strncmp(buffer, "220", 3) != 0) // If client does not send code 220, then disconnect
	{
		printf("Could not connect to server\n");
		close(server_sd);
		return 0;
	}
	printf("%s\n", buffer);

	// Communicate with server continuously

	while (1)
	{
		bzero(buffer, sizeof(buffer));

		printf("ftp> ");

		fgets(buffer, sizeof(buffer), stdin);

		buffer[strcspn(buffer, "\n")] = 0;

		if (strcmp(buffer, "") == 0)
			continue;

		int length = 0;

		char **input = splitString(buffer, sizeof(buffer), &length);

		if (!selectCommand(input, length, &state))
			continue; // Restart the loop if no data needs to be sent to server (e.g., in the case of an empty string)

		int send_bytes = send(server_sd, buffer, strlen(buffer), 0);

		handleTransfer(input, length, &state);

		// if (strcmp(buffer, "QUIT!") == 0) break; // We have an exit command for a disconnecdting client

		bzero(buffer, sizeof(buffer));

		int recv_bytes = read(server_sd, buffer, sizeof(buffer));

		if (DEBUG)
			printf("\n-------\nReceived Message: %s\nRecv Bytes: %d\n------\n", buffer, recv_bytes);
		printf("%s\n", buffer);

		if (recv_bytes == 0 || strcmp(buffer, QUITOK) == 0)
			break; // Stop communicating if the server sends no bytes
	}

	close(server_sd);
	return 1;
}
