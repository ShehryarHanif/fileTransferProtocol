#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int user(char **input, int length, struct State *state) // Handle the "USER" command
{
    if (length != 2){ // You must give the "USER" command and a username
        strcat(state->msg, "Invalid Number of Parameters");

        return 0;
    }

    if (!verifyUserExists(input[1])){ // A user has to exist to be logged in
        strcat(state->msg, "530 Not logged in.");
        return 0;
    }

    state->authenticated = 0; // The user needs to have the correct password to be authenticated

    strncpy(state->user, input[1], MAX_USER_SIZE);

    strcat(state->msg, "331 Username OK, need password.");

    return 1;
}

void createFolder(char *pwd) // We will need to create a folder for each user on the server side: https://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
{
    struct stat st = {0};

    if (stat(pwd, &st) == -1)
        mkdir(pwd, 0700);
}

int pass(char **input, int length, struct State *state) // Handle the "PASS" command
{
    if (length != 2) // // You must give the "PASS" command and a password
    {
        strcat(state->msg, "Invalid Number of Parameters"); // TODO
        return 0;
    }

    if (state->user == NULL || state->authenticated == 1) // A user needs to enter a username before the password can be entered
    {
        strcat(state->msg, "503 Bad sequence of commands.");

        return 0;
    }

    if (!verifyUserPassword(state->user, input[1])) // The password has to be valid
    {
        strcat(state->msg, "530 Not logged in.");

        return 0;
    }

    state->authenticated = 1; // Authenticate the user

    // A folder is created for a user if it does not exist

    strcpy(state->pwd, state->user);

    strcat(state->pwd, "/");

    createFolder(state->pwd);

    strcat(state->msg, "230 User logged in, proceed.");

    return 1;
}

int list(char **input, int length, struct State *state){ // Handle the "LIST" command
    // Reference: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program

    DIR *d;

    struct dirent *dir;

    d = opendir(state->pwd);

    if (d){
        // Ignore the ever-present "." and ".." directories

        dir = readdir(d);
        dir = readdir(d);

        while ((dir = readdir(d)) != NULL){ // Read each file one by one and send it
            bzero(state->msg, sizeof(state->msg));

            strcat(state->msg, dir->d_name);
            strcat(state->msg, "\n");

            int size = strlen(state->msg);

            send(state->ftp_client_connection, &size, sizeof(int), 0);
            send(state->ftp_client_connection, state->msg, strlen(state->msg), 0);
        }

        int falseFlag = 0;

        send(state->ftp_client_connection, &falseFlag, sizeof(int), 0);

        close(state->ftp_client_connection);

        closedir(d);

        char LISTOK[] = "226 Transfer completed.";


        send(state->client_sd, LISTOK, strlen(LISTOK), 0);
    } else {
        char FILENOTFOUND[] = "550 No File or Directory.";

        send(state->client_sd, FILENOTFOUND, strlen(FILENOTFOUND), 0);

        return 0;
    }

    return 1;
}

int pwd(char **input, int length, struct State *state){ // Handle the "PWD" command
    snprintf(state->msg, MAX_MESSAGE_SIZE, "257 %s", state->pwd);

    return 1;
}

int goBackFolder(char newPWD[MAX_LINUX_DIR_SIZE]){ // Used to move a folder back with "CWD"
    int length = strlen(newPWD);

    int i = length - 2;

    while (i >= 0){
        if (newPWD[i] == '/')
            break;

        newPWD[i--] = '\0';
    }
}

int cwd(char **input, int length, struct State *state) // Handle the "CWD" command{
    if (length != 2){ // You must give the "CWD" command and a directory
        strcat(state->msg, "Invalid number of arguments");

        return 0;
    }

    char *newDir = input[1];

    char newPWD[MAX_LINUX_DIR_SIZE];

    bzero(newPWD, MAX_LINUX_DIR_SIZE * sizeof(char));
    
    // Check if "newPWD" is a valid directory

    DIR *d;

    d = opendir(newPWD);

    if (!d){ // Handle the case with the wrong directory        
        strcpy(state->msg, "550 No such file or directory.");

        return 0;

    }

    // Update the present working directory

    bzero(state->pwd, sizeof(state->pwd));

    strcpy(state->pwd, newPWD);
    strcpy(state->msg, "200 directory changed to pathname/foldername.");

    return 1;
}

int openDataConnection(char *address, int length, struct State *state){ // Handle the creation of the data channel with the "PORT" command
    int ftp_connection = socket(AF_INET, SOCK_STREAM, 0);

    int value = 1;

    if (setsockopt(ftp_connection, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) < 0){
        perror("setsockopt failed");
        return 0;
    }

    if (ftp_connection < 0){
        perror("socket");

        exit(-1);
    }

    // Address of server

    struct sockaddr_in ftp_connection_addr;

    bzero(&ftp_connection_addr, sizeof(ftp_connection_addr));

    ftp_connection_addr.sin_family = AF_INET;
    ftp_connection_addr.sin_port = htons(DATA_PORT);
    ftp_connection_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(ftp_connection, (struct sockaddr *)&ftp_connection_addr, sizeof(ftp_connection_addr)) < 0)
    {
        perror("bind failed");
        return 0;
    }

    // Store the IP address and the port number as suggested in the given document(s)

    int h1, h2, h3, h4, p1, p2;

    char ipaddr[MAX_IPADDRSTR_SIZE];

    sscanf(address, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

    int port = p1 * 256 + p2;

    snprintf(ipaddr, MAX_IPADDRSTR_SIZE, "%d.%d.%d.%d", h1, h2, h3, h4);

    struct sockaddr_in ftp_connection_client_addr;

    bzero(&ftp_connection_client_addr, sizeof(ftp_connection_client_addr));

    ftp_connection_client_addr.sin_family = AF_INET;
    ftp_connection_client_addr.sin_port = htons(port);
    ftp_connection_client_addr.sin_addr.s_addr = inet_addr(ipaddr);

    // printf("Connecting on address: %s:%d\n", ipaddr, port);

    if (connect(ftp_connection, (struct sockaddr *)&ftp_connection_client_addr, sizeof(ftp_connection_client_addr)) < 0){
        perror("connect");

        strcpy(state->msg, "Could not establish FTP connection");

        return 0;
    }

    char PORTOK[] = "200 PORT command successful";

    strcpy(state->msg, PORTOK);

    state->ftp_client_connection = ftp_connection;

    return 1;
}

int exists(const char *fname) // Check if a file exists: https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
{
    FILE *file;

    if ((file = fopen(fname, "r"))){
        fclose(file);

        return 1;
    }

    return 0;
}

int stor(char **input, int length, struct State *state) // Handle the "STOR" command
{
    if (DEBUG)
        printf("A\n");

    if (length != 2){ // You must give the "STOR" command and a file
        strcpy(state->msg, "Invalid number of arguments");

        return 0;
    }
    else if (state->ftp_client_connection == -1) // Check if the data connection is valid, that is a user is logged in and other stuff has happened
    {
        strcpy(state->msg, "503 Bad sequence of commands.");

        return 0;
    }

    // Get the file name and use it to find where it is

    FILE *fptr;

    char filePath[MAX_LINUX_DIR_SIZE];

    snprintf(filePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, input[1]);

    char tempFilePath[MAX_LINUX_DIR_SIZE];

    // First, we find a random temporary file that can have stuff written to it.

    char randomFileName[sizeof(int) * 8 + 1];

    char randomFileNameExtension[] = ".tmp";

    snprintf(randomFileName, sizeof(randomFileName), "%d", rand());

    strcat(randomFileName, randomFileNameExtension);

    while(exists(randomFileName)){
        snprintf(randomFileName, sizeof(randomFileName), "%d", rand());

        strcat(randomFileName, randomFileNameExtension);
    }

    snprintf(tempFilePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, randomFileName);

    if (DEBUG)
        printf("C: File Path: %s\n", filePath);

    // Open the file and write to it

    if ((fptr = fopen(tempFilePath, "wb")) == NULL)
    {
        printf("550 No such file or directory.\n");
        return 0;
    }

    int recv_bytes;

    int file_length;

    int ftp_connection = state->ftp_client_connection;

    if (DEBUG)
        printf("D: ftp_connection\n");

    // First, get the length of the file

    recv_bytes = recv(ftp_connection, &file_length, sizeof(int), 0);

    // We will get the file packet by packet and will write to the file stream

    char buffer[PACKET_SIZE];

    int number_of_packets = file_length / PACKET_SIZE;

    printf("Size: %d\nNumber of packets: %d\nLoading bar: \n", file_length, number_of_packets);

    int i;

    for (i = 0; i < number_of_packets; i++)
    {
        recv_bytes = recv(ftp_connection, buffer, PACKET_SIZE, 0);

        while (recv_bytes < PACKET_SIZE)
        {
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], PACKET_SIZE-recv_bytes, 0);
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

        while (recv_bytes < remaining_bytes){
            recv_bytes += recv(ftp_connection, &buffer[recv_bytes], remaining_bytes-recv_bytes, 0);
        }
        fwrite(buffer, 1, remaining_bytes, fptr);
    }

    // Rename the file after the bytes have been written

    rename(tempFilePath, filePath);

    if (DEBUG)
        printf("E");

    close(ftp_connection);

    char OKMSG[] = "226 Transfer completed.";

    send(state->client_sd, OKMSG, strlen(OKMSG), 0);

    fclose(fptr);

    return 1;
}

int retr(char **input, int length, struct State *state) // Handle the "RETR" command
{
    if (length != 2){ // You must give the "RETR" command and a file
        strcpy(state->msg, "Invalid number of arguments");

        return 0;
    }
    else if (state->ftp_client_connection == -1) // Check if the data connection is valid, that is a user is logged in and other stuff has happened
    {
        strcpy(state->msg, "503 Bad sequence of commands.");

        return 0;
    }

    // See if a file exists and if it does, open and write to it

    char *fileName = input[1];

    char filePath[MAX_LINUX_DIR_SIZE];

    snprintf(filePath, MAX_LINUX_DIR_SIZE, "%s%s", state->pwd, fileName);

    FILE *fptr;

    if ((fptr = fopen(filePath, "rb")) == NULL){
        strcpy(state->msg, "550 No such file or directory.");

        char msg[] = "550 No such file or directory.";

        send(state->client_sd, msg, strlen(msg), 0);
        
        return 0;
    }

    // We will get the file packet by packet and will write to the file stream, but only after sending the length of the file

    int ftp_connection = state->ftp_client_connection;

    fseek(fptr, 0, SEEK_END);

    int file_length = ftell(fptr);

    send(ftp_connection, &file_length, sizeof(int), 0);

    fseek(fptr, 0, SEEK_SET);

    char buffer[PACKET_SIZE];

    int number_of_packets = file_length / PACKET_SIZE;

    int i;

    int send_bytes;

    for (i = 0; i < number_of_packets; i++){
        int start = i * PACKET_SIZE;

        fread(buffer, 1, PACKET_SIZE, fptr);

        send_bytes = send(ftp_connection, buffer, PACKET_SIZE, 0);
    }

    // There might be some bytes left over that still need to be written, so write them

    int remaining_bytes = file_length - number_of_packets * PACKET_SIZE;

    int start = i * PACKET_SIZE;

    if (remaining_bytes > 0){
        bzero(buffer, PACKET_SIZE);
        fread(buffer, 1, remaining_bytes, fptr);
        send(ftp_connection, buffer, remaining_bytes, 0);
    }

    close(state->ftp_client_connection);

    state->ftp_client_connection = -1;

    printf("226 Transfer completed.");

    char OKMSG[] = "226 Transfer completed.";

    send(state->client_sd, OKMSG, strlen(OKMSG), 0);

    fclose(fptr);

    return 1;
}

int selectCommand(char buffer[], int buffer_size, struct State *state){ // Handle all the commands needed by the user on the server side
    int length = 0;

    char **input = splitString(buffer, buffer_size, &length);

    if (length == 0)
        return 1;

    bzero(state->msg, MAX_MESSAGE_SIZE * sizeof(char));

    char *command = input[0];

    if (command == NULL)
    {
        printf("Error: Null Command \n");
        return 0;
    }
    else if (strcmp(command, "USER") == 0)
    {
        user(input, length, state);
        return 1;
    }
    else if (strcmp(command, "PASS") == 0)
    {
        pass(input, length, state);
        return 1;
    }
    else if (!(state->authenticated)) // Non-authentication will not allow certain commands
    {
        strcat(state->msg, "530 Not logged in.");

        return 1;
    }
    else if (strcmp(command, "PORT") == 0)
    {
        if (!openDataConnection(input[1], length, state))
        {
            strcpy(state->msg, "200 PORT command successful");
        }

        return 1;
    }
    else if (strcmp(command, "STOR") == 0) // Fork to handle different clients
    {
        int pid = fork();

        if (pid == 0)
        {
            stor(input, length, state);

            exit(1);
        }

        return 0;
    }
    else if (strcmp(command, "RETR") == 0) // Fork to handle different clients
    {
        int pid = fork();

        if (pid == 0)
        {
            retr(input, length, state);

            exit(1);
        }

        return 0;
    }
    else if (strcmp(command, "LIST") == 0) // Fork to handle different clients
    {
        int pid = fork();

        if (pid == 0)
        {
            list(input, length, state);

            exit(1);
        }

        return 0;
    }
    else if (strcmp(command, "CWD") == 0)
    {
        cwd(input, length, state);

        return 1;
    }
    else if (strcmp(command, "PWD") == 0)
    {
        pwd(input, length, state);
        
        return 1;
    }
    else
    {
        strcpy(state->msg, "202 Command not implemented.");

        return 1;
    }
}