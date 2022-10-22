#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>

#include<unistd.h>
#include<stdlib.h>

#include "globals.h"
#include "client_state.h"
#include "server_input.h"
#include "client_commands.h"


// Code template from: rn3_simple_client.c by Rohail Asim

int main()
{
	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}

    // Address of server
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONTROL_PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct State state = {"","",-1};
	if(!initializePWD(&state)){
		perror("Could not initialize state!");
		return 0;
	}

	// connecting
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
	printf("Connected to server\n");
	
	char buffer[4096];
	struct sockaddr_in client_addr;
	int client_size = sizeof(client_addr);
	getsockname(server_sd, &client_addr,  &client_size);

	strncpy(state.ipaddr, inet_ntoa(client_addr.sin_addr), MAX_IPADDRSTR_SIZE);
	state.port = ntohs(client_addr.sin_port);
	// snprintf(state.port, MAX_PORTSTR_SIZE, "%d", ntohs(client_addr.sin_port));
	// strncpy(state.port, itoa(ntohs(client_addr.sin_port)), MAX_PORTSTR_SIZE);
	// printf("[%s:%s]: ",state.ipaddr, state.port); 

	// Communicate with server continuously
	while (1) {
		bzero(buffer,sizeof(buffer));
		
		printf("ftp> ");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = 0;

		if (strcmp(buffer, "")==0) continue;

		int length = 0;
		char** input = splitString(buffer, sizeof(buffer), &length);

		if (!selectCommand(input, length, &state, server_sd)) continue; // restart loop if no data needs to be sent to server
		
		int send_bytes = send(server_sd, buffer, strlen(buffer), 0);

		if (strcmp(buffer, "QUIT!") == 0) break;

		bzero(buffer,sizeof(buffer));
		int recv_bytes = read(server_sd, buffer, sizeof(buffer));
		printf("Received Message: %s\nRecv Bytes: %d\n", buffer, recv_bytes);

		if (recv_bytes==0) break; // Stop communicating if the server returns no bytes. Could show server is no longer running
	}
	close(server_sd);
}
