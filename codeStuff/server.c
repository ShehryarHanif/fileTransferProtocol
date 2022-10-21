#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <unistd.h>
#include <stdlib.h>

#define DEBUG 1

#include "server_state.h"
#include "server_input.h"
#include "server_users.h"

#include "server_commands.h"

// Code template from: rn3_simple_server_updated.c by Rohail Asim

int main()
{
	int server_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sd < 0)
	{
		perror("socket:");
		exit(-1);
	}
	// setsock
	int value = 1;
	setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));

	// address
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5001);
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
			printf("FD in question: %d\n", fd);
			if (FD_ISSET(fd, &ready_fdset))
			{
				printf("FD Set!: %d\n", fd);
				if (fd == server_sd)
				{
					socklen_t slen = sizeof(client_addr);
					client_sd = accept(server_sd, (struct sockaddr *)&client_addr, &slen);

					printf("[%s:%d]: ", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					printf("Client Connected, fd: %d\n", client_sd);

					char buffer[256]; // Buffer to hold communicated data

					FD_SET(client_sd, &full_fdset);
					if (client_sd > max_fd){
						max_fd = client_sd; // Update the max_fd if new socket has higher FD
						printf("Setup: maxfd: %d\n", max_fd);
					}
						
				}
				else
				{
					char buffer[4096];

					bzero(buffer, sizeof(buffer)); // Clear values in buffer to 0
					int recv_bytes = recv(fd, buffer, sizeof(buffer), 0);

					if (recv_bytes == 0)
					{
						close(fd); // close the copy of client/secondary socket in parent process

						FD_CLR(fd, &full_fdset);

						break; // If no bytes were received, then break loop and end communication
					}

					send(fd, buffer, strlen(buffer), 0);
					printf("Send Message: %s\n", buffer);
				}
			}
		}
	}
	close(server_sd);
}
