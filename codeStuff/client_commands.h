#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TRIES 100
#define FTP_ACCEPT_TIMEOUT 5

int pwd(int length, struct State *state) // Handle the "!PWD" command
{
    if (length != 1)
    { // The user needs to enter just the "PWD" command
        printf("Invalid number of arguments\n");

        return 0;
    }

    printf("%s\n", state->pwd);

    return 1;
}

int goBackFolder(char newPWD[MAX_LINUX_DIR_SIZE]) // If the argument of cwd is "..", we want to manually edit the "PWD"" string
{
    int length = strlen(newPWD);

    int i = length - 2;

    while (i >= 0)
    {
        if (newPWD[i] == '/')
            break;

        newPWD[i--] = '\0';
    }
}

int cwd(char **input, int length, struct State *state) // Handle the "!PWD" command
{
    if (length != 2)
    { // The user needs to enter the "!CWD" command and the folder name
        printf("Invalid number of arguments\n");

        return 0;
    }

    char *newDir = input[1];

    char newPWD[MAX_LINUX_DIR_SIZE];

    bzero(newPWD, MAX_LINUX_DIR_SIZE * sizeof(char));

    strcpy(newPWD, state->pwd);
    strcat(newPWD, newDir);

    // Resolving ".."s and "."s in the path

    realpath(newPWD, newPWD);

    strcat(newPWD, "/"); // Adding extra "/" at the end to close off the path

    // Check if "newPWD" is a valid dir

    DIR *d;

    d = opendir(newPWD);

    if (!d)
    {
        // given folder does not exist
        printf("Folder not found\n"); // TODO
        return 0;
    }

    bzero(state->pwd, sizeof(state->pwd));

    strcpy(state->pwd, newPWD);

    return 1;
}

int list(int length, struct State *state)
{ // Handle the "!LIST" command
    // Reference: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program

    if (length != 1)
    { // The user needs to enter just the "LIST" command
        printf("Invalid number of arguments\n");

        return 0;
    }

    DIR *d;

    struct dirent *dir;

    d = opendir(state->pwd);

    if (d)
    {
        // Ignore the ever-present "." and ".." directories

        dir = readdir(d);
        dir = readdir(d);

        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }

        closedir(d);
    }
    else
    {
        printf("Folder does not exist\n");

        return 0;
    }

    return 1;
}

int create_socket(struct State *state, int *port, int *ftp_connection)
{ // Handle the creation of the data channel's socket
    if (DEBUG)
        printf("PORT: Creating Socket...\n");

    int initial_base_port = state->port;

    *ftp_connection = socket(AF_INET, SOCK_STREAM, 0);

    int value = 1;

    if (setsockopt(*ftp_connection, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) < 0)
    {
        perror("setsockopt failed");

        return 0;
    }

    struct timeval tv;

    tv.tv_sec = FTP_ACCEPT_TIMEOUT;

    tv.tv_usec = 0;

    if (setsockopt(*ftp_connection, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0)
    {
        perror("setsockopt failed");

        return 0;
    }

    struct sockaddr_in ftp_connection_addr;

    bzero(&ftp_connection_addr, sizeof(ftp_connection_addr));

    ftp_connection_addr.sin_family = AF_INET;
    ftp_connection_addr.sin_addr.s_addr = inet_addr(state->ipaddr);

    // The port needs to be changed in case there is usage already

    int base_port = ((initial_base_port + state->displacement - 1025) % 64510) + 1025;

    state->displacement = (state->displacement % 100) + 1;

    ftp_connection_addr.sin_port = htons(base_port);

    if (DEBUG)
        printf("PORT: Binding..., %s:%d\n", state->ipaddr, base_port);

    int tries = 0;

    while (bind(*ftp_connection, (struct sockaddr *)&ftp_connection_addr, sizeof(ftp_connection_addr)) < 0 && tries < MAX_TRIES)
    {

        if (DEBUG)
            printf("Errno Numbers: %d\n", errno);

        if (errno == EADDRINUSE || errno == EADDRNOTAVAIL || errno == EACCES)
        {
            base_port = ((initial_base_port - 1025 + state->displacement) % 64510) + 1025;

            state->displacement = (state->displacement % 100) + 1;

            tries++;

            ftp_connection_addr.sin_port = htons(++base_port);
        }
        else
        {
            perror("bind failed");
            return 0;
        }
    }

    if (tries == MAX_TRIES) // We limit the maximum number of ports that can be used by a client
    {
        printf("Range of ports allocated for client are not all not available\n");

        return 0;
    }

    if (DEBUG)
        printf("PORT: Finished Binding!\n");

    *port = base_port;

    if (listen(*ftp_connection, 1) < 0)
    {
        perror("listen failed");

        close(*ftp_connection);

        return 0;
    }

    return 1;
}

int port(struct State *state)
{ // Handle the "PORT" command
    int port = 0;

    int ftp_connection = 0;

    int success = create_socket(state, &port, &ftp_connection);

    if (!success)
        return 0;

    // convert the port and IP addresses to bytes

    char ipaddrCopy[MAX_IPADDRSTR_SIZE];

    strcpy(ipaddrCopy, state->ipaddr);

    char *h1 = strtok(ipaddrCopy, ".");
    char *h2 = strtok(NULL, ".");
    char *h3 = strtok(NULL, ".");
    char *h4 = strtok(NULL, ".");

    int p1 = port / 256;
    int p2 = port % 256;

    char portCommand[30];

    snprintf(portCommand, 30, "PORT %s,%s,%s,%s,%d,%d", h1, h2, h3, h4, p1, p2);

    if (DEBUG)
        printf("Command: %s\n", portCommand);

    send(state->server_sd, portCommand, strlen(portCommand), 0);

    char buffer[4096];
    bzero(buffer, sizeof(buffer));

    int recv_bytes = read(state->server_sd, buffer, sizeof(buffer));

    printf("%s\n", buffer);

    if (strncmp(buffer, "200", 3) != 0) return 0;

    state->ftp_connection = ftp_connection;

    return 1;
}

int acceptDataConnection(struct State* state){

    if (state->ftp_connection == -1) 
    {
        printf("Cannot accept data connection without opening one first\n");
        return 0;
    }

    char buffer[4096];
    bzero(buffer, sizeof(buffer));

    int recv_bytes = read(state->server_sd, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    if (strncmp(buffer, "150", 3) != 0) return 0;


    struct sockaddr_in client_addr;

    socklen_t slen = sizeof(client_addr);

    int ftp_client_connection;

    if ((ftp_client_connection = accept(state->ftp_connection, (struct sockaddr *)&client_addr, &slen)) < 0)
    {
        close(state->ftp_connection);
        // Receive msg if available
        return 0;
    }
    if (DEBUG)
        printf("FTP CLIENT ADDRESS: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    

    state->ftp_client_connection = ftp_client_connection;

    close(state->ftp_connection);
    state->ftp_connection = -1;

    return 1;
}

int selectCommand(char **input, int length, struct State *state) // Handle user commands, noting that directory manipulation is allowed without authentication (there was no specification given regarding this)
{
    char *command = input[0];

    if (strcmp(command, "!CWD") == 0)
    {
        cwd(input, length, state);
        return 0;
    }
    else if (strcmp(command, "!PWD") == 0)
    {
        pwd(length, state);
        return 0;
    }
    else if (strcmp(command, "!LIST") == 0)
    {
        list(length, state);
        return 0;
    }
    else if (strcmp(command, "LIST") == 0 || strcmp(command, "STOR") == 0 || strcmp(command, "RETR") == 0) // These are commands handled at the server side
    {
        return port(state);
    }
    else
    {
        return 1;
    }
}

int listServer(char **input, int length, struct State *state) // Handle the "LIST" command
{
    if (DEBUG)
        printf("A\n");

    if (length != 1)
    {
        return 0;
    }

    if (DEBUG)
        printf("B\n");

    int recv_bytes;

    int ftp_connection = state->ftp_client_connection;

    if (DEBUG)
        printf("D: ftp_connection\n");

    // Get files one by one and display them

    char buffer[PACKET_SIZE];

    int stringLength;

    while (((recv_bytes = recv(ftp_connection, &stringLength, sizeof(int), 0)) > 0) && (stringLength != 0))
    {
        bzero(buffer, PACKET_SIZE);

        recv_bytes = recv(ftp_connection, buffer, stringLength, 0);

        printf("%s", buffer);
    }

    fflush(stdout);

    if (DEBUG)
        printf("E");

    close(ftp_connection);

    return 1;
}

int exists(const char *fname) // Check if a file exists: https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
{
    FILE *file;

    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }

    return 0;
}

int stor(char **input, int length, struct State *state) // Handle the "STOR" command
{
    if (state->ftp_client_connection == -1) // Check if the data connection is valid, that is a user is logged in and other stuff has happened
    {
        printf("FTP Connection not established\n");

        return 0;
    }
    else if (length != 2)
    { // You must give the "STOR" command and a file
        // printf("Invalid number of arguments\n");

        close(state->ftp_client_connection);

        return 0;
    }

    // Read the file from the user's end and send it to the server

    char *fileName = input[1];

    char filePath[MAX_LINUX_DIR_SIZE];

    snprintf(filePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, fileName);

    FILE *fptr;

    if ((fptr = fopen(filePath, "rb")) == NULL)
    {
        printf("File not found\n");

        send(state->server_sd, -1, sizeof(int), 0);

        close(state->ftp_client_connection);

        state->ftp_client_connection = -1;

        return 0;
    }

    int ftp_connection = state->ftp_client_connection;

    fseek(fptr, 0, SEEK_END);

    int file_length = ftell(fptr);

    send(ftp_connection, &file_length, sizeof(int), 0);

    fseek(fptr, 0, SEEK_SET);

    // We will get the file packet by packet and will write to the file stream

    char buffer[PACKET_SIZE];

    int number_of_packets = file_length / PACKET_SIZE;

    int i;

    int send_bytes;

    for (i = 0; i < number_of_packets; i++)
    {
        int start = i * PACKET_SIZE;
        fread(buffer, 1, PACKET_SIZE, fptr);
        send_bytes = send(ftp_connection, buffer, PACKET_SIZE, 0);

        if (send_bytes <= 0)
            printf("SEND FAIL: Failed to send Packet!\n");
    }

    // There might be some bytes left over that still need to be written, so write them

    int remaining_bytes = file_length - number_of_packets * PACKET_SIZE;

    if (remaining_bytes > 0)
    {
        bzero(buffer, PACKET_SIZE);

        fread(buffer, 1, remaining_bytes, fptr);

        send(ftp_connection, buffer, remaining_bytes, 0);
    }

    close(state->ftp_client_connection);

    state->ftp_client_connection = -1;

    fclose(fptr);
    return 1;
}

int retr(char **input, int length, struct State *state) // Handle the "RETR" command
{
    if (DEBUG)
        printf("A\n");

    if (length != 2)
    {
        return 0;
    }

    if (DEBUG)
        printf("B\n");

    FILE *fptr;

    char filePath[MAX_LINUX_DIR_SIZE];

    snprintf(filePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, input[1]);

    char tempFilePath[MAX_LINUX_DIR_SIZE];

    // We can have multiple clients writing to the same directory, so generate random numbers for the file names

    char randomFileName[sizeof(int) * 8 + 1];

    char randomFileNameExtension[] = ".tmp";

    snprintf(randomFileName, sizeof(randomFileName), "%d", rand());

    strcat(randomFileName, randomFileNameExtension);

    while (exists(randomFileName))
    {
        snprintf(randomFileName, sizeof(randomFileName), "%d", rand());

        strcat(randomFileName, randomFileNameExtension);
    }

    snprintf(tempFilePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, randomFileName);

    int recv_bytes;
    int file_length;

    int ftp_connection = state->ftp_client_connection;

    char c[1];

    if (DEBUG)
        printf("D: ftp_connection\n");

    recv_bytes = recv(ftp_connection, &file_length, sizeof(int), 0);

    if (recv_bytes < 0)
    {
        close(state->ftp_client_connection);

        state->ftp_client_connection = -1;

        return 0;
    }

    if (DEBUG)
        printf("C: File Path: %s\n", filePath);
    if ((fptr = fopen(tempFilePath, "wb")) == NULL)
    {
        printf("File not found\n");

        // send(state->server_sd, -1, sizeof(int), 0);

        close(state->ftp_client_connection);

        state->ftp_client_connection = -1;

        return 0;
    }

    // We will get the file packet by packet and will write to the file stream

    char buffer[PACKET_SIZE];

    int number_of_packets = file_length / PACKET_SIZE;

    int i;

    for (i = 0; i < number_of_packets; i++)
    {
        recv_bytes = recv(ftp_connection, buffer, PACKET_SIZE, 0);
        while (recv_bytes < PACKET_SIZE)
        {
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], PACKET_SIZE - recv_bytes, 0);
        }

        fwrite(buffer, 1, PACKET_SIZE, fptr);
    }

    // There might be some bytes left over that still need to be written, so write them

    int remaining_bytes = file_length - number_of_packets * PACKET_SIZE;

    int start = i * PACKET_SIZE;

    if (remaining_bytes > 0)
    {
        bzero(buffer, PACKET_SIZE);

        recv_bytes = recv(ftp_connection, buffer, remaining_bytes, 0);

        while (recv_bytes < remaining_bytes)
        {
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], remaining_bytes - recv_bytes, 0);
        }

        fwrite(buffer, 1, remaining_bytes, fptr);
    }

    // Rename the file after the bytes have been written

    rename(tempFilePath, filePath);

    if (DEBUG)
        printf("E\n");
    close(ftp_connection);
    fclose(fptr);
    return 1;
}

int handleTransfer(char **input, int length, struct State *state) // Handle the commands for the data channel
{
    char *command = input[0];

    if (strcmp(command, "STOR") == 0)
    {
        if (!acceptDataConnection(state)) return 1;
        stor(input, length, state);

        return 1;
    }
    else if (strcmp(command, "RETR") == 0)
    {
        if (!acceptDataConnection(state)) return 1;
        retr(input, length, state);

        return 1;
    }
    else if (strcmp(command, "LIST") == 0)
    {
        if (!acceptDataConnection(state)) return 1;
        listServer(input, length, state);

        return 1;
    }

    return 1;
}