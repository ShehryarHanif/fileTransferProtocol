// #define _OPEN_SYS_ITOA_EXT

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TRIES 100
#define FTP_ACCEPT_TIMEOUT 10

// srand(time(NULL));

// check if local command
int isLocalCommand(char *input)
{
    return input[0] == '!';
}

int pwd(struct State *state)
{
    printf("%s\n", state->pwd);
    return 1;
}

// if the argument of cwd is "..", we want to manually edit the pwd string
int goBackFolder(char newPWD[MAX_LINUX_DIR_SIZE])
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

int cwd(char **input, int length, struct State *state)
{
    if (length != 2)
    {
        printf("Invalid number of arguments");
        return 0;
    }
    char *newDir = input[1];
    char newPWD[MAX_LINUX_DIR_SIZE];
    bzero(newPWD, MAX_LINUX_DIR_SIZE * sizeof(char));
    // We have not considered the cases when newDir == "../.", "./../.././.." or any such combinations
    if (strcmp(newDir, "..") == 0)
    {
        strcpy(newPWD, state->pwd);
        goBackFolder(newPWD);
    }
    else if (strcmp(newDir, ".") == 0)
    {
        return 1;
    }
    else
    {
        strcpy(newPWD, state->pwd);
        strcat(newPWD, newDir);
        strcat(newPWD, "/");
    }

    // Check if newPWD is a valid dir
    DIR *d;
    d = opendir(newPWD);
    if (!d)
    {
        // given folder does not exist
        // TODO: Proper error msg
        printf("Folder `%s` does not exist!\n", newPWD);
        return 0;
    }
    bzero(state->pwd, sizeof(state->pwd));
    strcpy(state->pwd, newPWD);
    return 1;
}

int list(struct State *state)
{
    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;
    d = opendir(state->pwd);
    // bzero(state->msg);
    if (d)
    {
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
        // TODO: Figure this out
        // directory does not exist (if errno == ENOENT) with #include <errno.h>
        printf("Folder does not exist\n");
        return 0;
    }
    return 1;
}

int create_socket(struct State *state, int *port, int *ftp_connection)
{
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
            printf("PORT: Unavailable Port %d\n", base_port);

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

    if (tries == MAX_TRIES)
    {
        printf("Range of ports allocated for client are not all not available");
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
    // Accept single connection
    // struct sockaddr_in client_addr;
    return 1;
}

int port(struct State *state)
{
    // create socket for data transfer
    int port = 0;
    int ftp_connection = 0;
    int success = create_socket(state, &port, &ftp_connection); // get ip address and port
    if (!success)
        return 0;
    // convert port and ip addresses to single bytes
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
    printf("Command: %s\n", portCommand);

    send(state->server_sd, portCommand, strlen(portCommand), 0);

    struct sockaddr_in client_addr;
    socklen_t slen = sizeof(client_addr);
    int ftp_client_connection;
    if ((ftp_client_connection = accept(ftp_connection, (struct sockaddr *)&client_addr, &slen)) < 0)
    {
        close(ftp_connection);
        return 0;
    }
    if (DEBUG)
        printf("FTP CLIENT ADDRESS: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    char buffer[4096];
    bzero(buffer, sizeof(buffer));
    int recv_bytes = read(state->server_sd, buffer, sizeof(buffer));
    printf("Received ftp message: %s\nBytes: %d\n", buffer, recv_bytes);

    state->ftp_client_connection = ftp_client_connection;
    // close(ftp_client_connection);

    close(ftp_connection);
    return 1;
}

// returns 1 if needs to send data to server
// returns 0 if no data needs to be sent
// TODO: Add note for this design choice
// ----- Directory manipulation without login
int selectCommand(char **input, int length, struct State *state)
{
    char *command = input[0];
    if (strcmp(command, "!CWD") == 0)
    {
        cwd(input, length, state);
        return 0;
    }
    else if (strcmp(command, "!PWD") == 0)
    {
        pwd(state);
        return 0;
    }
    else if (strcmp(command, "!LIST") == 0)
    {
        list(state);
        return 0;
    }
    else if (strcmp(command, "STOR") == 0 || strcmp(command, "RETR") == 0)
    {
        // PORT
        if (!port(state))
            return 0;
        return 1;
    }
    // else if (strcmp(command, "RETR") == 0)
    // {
    //     // send port command first
    //     return 0;
    // }
    else
    {
        return 1;
        printf("Invalid Local Command\n");
    }
}

int stor(char **input, int length, struct State *state)
{
    // Variable: Ftp_connection variable
    // Fork
    printf("\n--------\nStarting retr...\n");
    if (length != 2)
    {
        printf("Invalid number of arguments");
        return 0;
    }
    else if (state->ftp_client_connection == -1) // Check if data connection is valid
    {
        printf("FTP Connection not established");
        return 0;
    }
    char *fileName = input[1];

    char filePath[MAX_LINUX_DIR_SIZE];

    snprintf(filePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, fileName);

    FILE *fptr;
    if ((fptr = fopen(filePath, "rb")) == NULL)
    {
        // printf("File not found\n");
        printf("File not found");
        // char msg[] = "File not found";
        // send(state->server_sd, msg, strlen(msg), 0);
        send(state->server_sd, -1, sizeof(int), 0);
        return 0;
    }

    int ftp_connection = state->ftp_client_connection;

    fseek(fptr, 0, SEEK_END);
    int file_length = ftell(fptr);
    printf("File Length: %d\n", file_length);
    send(ftp_connection, &file_length, sizeof(int), 0);
    fseek(fptr, 0, SEEK_SET);

    char buffer[PACKET_SIZE];
    
    int number_of_packets = file_length / PACKET_SIZE;

    printf("Size: %d\nNumber of packets: %d\n", file_length, number_of_packets);

    int i;
    int send_bytes;
    for (i = 0; i < number_of_packets; i++)
    {
        int start = i * PACKET_SIZE;
        fread(buffer, 1, PACKET_SIZE, fptr);
        send_bytes = send(ftp_connection, buffer, PACKET_SIZE, 0);
        if (send_bytes != PACKET_SIZE)
            printf("SEND FAIL: Imbalanced Send\n");
        if (send_bytes <= 0)
            printf("SEND FAIL: Failed to send Packet!\n");
    }

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

int retr(char **input, int length, struct State *state)
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
    char randomFileName[sizeof(int) * 8 + 1];
    // itoa(rand(), randomFileName, DECIMAL);

    snprintf(randomFileName, sizeof(randomFileName), "%d", rand());

    strcat(randomFileName, ".tmp");

    snprintf(tempFilePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, randomFileName);

    if (DEBUG)
        printf("C: File Path: %s\n", filePath);
    if ((fptr = fopen(tempFilePath, "wb")) == NULL)
    {
        printf("File not found\n");
        return 0;
    }

    int recv_bytes;
    int file_length;
    int ftp_connection = state->ftp_client_connection;
    char c[1];
    if (DEBUG)
        printf("D: ftp_connection\n");
    recv_bytes = recv(ftp_connection, &file_length, sizeof(int), 0);

    char buffer[PACKET_SIZE];

    // recv_bytes = recv(ftp_connection, buffer, file_size, 0);
    int number_of_packets = file_length / PACKET_SIZE;

    // char bg = 177;
    // char load = 219;
    // for (int i = 0; i < 100; i++)
    //     printf("-");

    // int packet_per_bar = number_of_packets / 64000 + 1;

    printf("Size: %d\nNumber of packets: %d\nLoading bar: \n", file_length, number_of_packets);
    int i;
    for (i = 0; i < number_of_packets; i++)
    {
        recv_bytes = recv(ftp_connection, buffer, PACKET_SIZE, 0);
        while (recv_bytes < PACKET_SIZE)
        {
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], PACKET_SIZE - recv_bytes, 0);
            // printf("RECEIVE FAIL: Imbalanced recv \n");
        }

        fwrite(buffer, 1, PACKET_SIZE, fptr);
    }

    int remaining_bytes = file_length - number_of_packets * PACKET_SIZE;

    int start = i * PACKET_SIZE;
    if (remaining_bytes > 0)
    {
        bzero(buffer, PACKET_SIZE);
        recv_bytes = recv(ftp_connection, buffer, remaining_bytes, 0);
        while (recv_bytes < remaining_bytes)
        {
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], remaining_bytes - recv_bytes, 0);
            // printf("RECEIVE FAIL: Imbalanced recv \n");
        }
        fwrite(buffer, 1, remaining_bytes, fptr);
    }

    // fwrite(buffer, 1, file_size, fptr);

    // do {
    //     recv_bytes = recv(ftp_connection, c, sizeof(char), 0);
    //     if (recv_bytes==0) break;
    //     printf("Received: %d\nChar: %c\n", recv_bytes, c[0]);
    //     fputc(c[0], fptr);
    //     // fwrite(&c[0], 1, sizeof(char), fptr);
    // } while (recv_bytes > 0);

    rename(tempFilePath, filePath);

    if (DEBUG)
        printf("E");
    close(ftp_connection);
    fclose(fptr);
    return 1;
}

int handleTransfer(char **input, int length, struct State *state)
{
    char *command = input[0];
    if (strcmp(command, "STOR") == 0)
    {
        stor(input, length, state);
        return 1;
    }
    else if (strcmp(command, "RETR") == 0)
    {
        retr(input, length, state);
        return 1;
    }
    return 1;
}