#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <unistd.h>
#include <stdlib.h>

#define DEBUG 1
#define MAX_CLIENTS 5

#include "globals.h"
#include "server_state.h"
#include "server_input.h"
#include "server_users.h"

#include "server_commands.h"

// Code template from: rn3_simple_server_updated.c by Rohail Asim

int main()
{
	if (!readUsers()) return 0; // Stop if there are problems reading the file

	int server_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sd < 0)
	{
		perror("socket error:");
		exit(-1);
	}
	// setsock
	int value = 1;
	setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));

	// setsock for timeout
	struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(server_sd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0){
        perror("setsockopt timeout failed");
        return 0;
    }

	// address
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONTROL_PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// bind
	if (bind(server_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind failed");
		exit(-1);
	}
	// listen
	if (listen(server_sd, 5) < 0)
	{
		perror("listen failed");
		close(server_sd);
		exit(-1);
	}

	printf("Server is listening...\n");

	fd_set full_fdset, ready_fdset;

	FD_ZERO(&full_fdset);
	FD_SET(server_sd, &full_fdset);

	int max_fd = server_sd;
	int initial_max_fd = max_fd;
	// Accepting any number of clients
	while (1)
	{

		// Create a separate process to interact with that particular client
		ready_fdset = full_fdset;

		if (select(max_fd + 1, &ready_fdset, NULL, NULL, NULL) < 0)
		{
			perror("select");
			return -1;
		}

		int client_sd;
		struct sockaddr_in client_addr;
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &ready_fdset))
			{
				if (fd == server_sd)
				{
					socklen_t slen = sizeof(client_addr);
					client_sd = accept(server_sd, (struct sockaddr *)&client_addr, &slen);

					// `client_sd - initial_max_fd` represents the number of clients in connection
					if (client_sd - initial_max_fd > MAX_CLIENTS){
						close(client_sd);
						continue;
					}
					

					printf("[%s:%d]: Client Connected, fd: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_sd);

					char buffer[256]; // Buffer to hold communicated data

					FD_SET(client_sd, &full_fdset);
					if (client_sd > max_fd){
						max_fd = client_sd; // Update the max_fd if new socket has higher FD
					}
					int state_index = client_sd - initial_max_fd - 1;
					// `fd - initial_max_fd - 1` represents the index of clients starting from 0
					resetState(&state[state_index]);
					state[state_index].client_sd = client_sd;

					// Auto login:
					strcpy(state[state_index].user, "bob");
					state[state_index].authenticated = 1;
					sprintf(state->pwd, "%s/", "bob");
				}
				else
				{
					char buffer[4096];
					int state_index = fd - initial_max_fd - 1;
					// `fd - initial_max_fd - 1` represents the index of clients starting from 0

					bzero(buffer, sizeof(buffer)); // Clear values in buffer to 0

					int recv_bytes = recv(fd, buffer, 4096, 0);

					if (recv_bytes == 0 || strcmp(buffer, "QUIT!") == 0)
					{
						printf("Closing connection to client\n");
						resetState(&state[state_index]);
						close(fd); // close the copy of client/secondary socket in parent process

						FD_CLR(fd, &full_fdset);

						// break; // If no bytes were received, then break loop and end communication
						continue;
					}
					int shouldSendData = selectCommand(buffer, 4096, &state[state_index]);

					if (!shouldSendData) continue;
					
					// send(fd, state[state_index].msg, MAX_MESSAGE_SIZE*sizeof(char), 0);
					if (strcmp(state[state_index].msg, "")==0) strcpy(state[state_index].msg, " ");
					write(fd, state[state_index].msg, strlen(state[state_index].msg));
					if (DEBUG) printf("Sent Message: \n%s\n", state[state_index].msg);
				}
			}
		}
	}
	close(server_sd);
}
