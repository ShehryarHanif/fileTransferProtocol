#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>

#include<unistd.h>
#include<stdlib.h>


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
	server_addr.sin_port = htons(5001);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// connecting
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
	printf("Connected to server\n");
	
	char buffer[256];

	// Communicate with server continuously
	while (1){
		bzero(buffer,sizeof(buffer));
		printf("Enter a message: ");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = 0;
		
		int send_bytes = send(server_sd, buffer, strlen(buffer), 0);

		if (strcmp(buffer, "BYE!")==0) break; // Stop communicating if "BYE!" is typed

		bzero(buffer,sizeof(buffer));
		int recv_bytes = recv(server_sd, buffer, sizeof(buffer), 0);
		printf("Received Message: %s\n", buffer);

		if (recv_bytes==0) break; // Stop communicating if the server returns no bytes. Could show server is no longer running
	}
	close(server_sd);
}
