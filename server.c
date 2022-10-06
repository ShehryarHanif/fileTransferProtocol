#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>

#include<unistd.h>
#include<stdlib.h>

// Code template from: rn3_simple_server_updated.c by Rohail Asim

int main()
{
	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}
	//setsock
	int value = 1;
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(int)); 

	// address
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5001);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

	//bind
	if(bind(server_sd, (struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		perror("bind failed");
		exit(-1);
	}
	//listen
	if(listen(server_sd,5)<0)
	{
		perror("listen failed");
		close(server_sd);
		exit(-1);
	}
	

	printf("Server is listening...\n");

	// Accepting any number of clients
	while (1) {
		//accept
		struct sockaddr_in client_addr;
		socklen_t slen = sizeof(client_addr);
		int client_sd = accept(server_sd,(struct sockaddr *)&client_addr, &slen);

		// Create a separate process to interact with that particular client
		int pid = fork();
		
		if (pid == 0) {
			close(server_sd); //close the copy of server/master socket in child process
			printf("[%s:%d]: ",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			printf("Client Connected\n");
			
			char buffer[256]; // Buffer to hold communicated data

			do {
				bzero(buffer,sizeof(buffer)); // Clear values in buffer to 0

				int recv_bytes = recv(client_sd, buffer, sizeof(buffer), 0);
				printf("[%s:%d]: ",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
				printf("Received Message: %s\n",buffer);

				if (recv_bytes==0) break; // If no bytes were received, then break loop and end communication
				
				send(client_sd, buffer, strlen(buffer), 0);
				printf("[%s:%d]: ",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
				printf("Send Message: %s\n",buffer);

			} while (strcmp(buffer, "BYE!")!=0); // exit if client types "BYE!""
			break;
		} else {
			close(client_sd);  //close the copy of client/secondary socket in parent process
		}
	}

	//close
	printf("Disconnected\n");
	close(server_sd);
}
