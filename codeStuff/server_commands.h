// commands to handle:
// 1. PORT
// 2. STOR
// 3. USER
// 4. PASS
// 5. RETR
// 6. LIST !
// 7. CWD !
// 8. PWD !
// 9. QUIT
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

// returns 1 if succesful
// returns 0 otherwise
int user(char **input, int length, struct State *state)
{

    // If the number of arguments is not 2 (USER + "<username>"),
    // or if the user does not exist
    // Then the argument fails
    if (length != 2)
    {
        // Error msg: Invalid Number of Parameters
        strcat(state->msg, "Invalid Number of Parameters");
        return 0;
    }
    if (!verifyUserExists(input[1]))
    {
        strcat(state->msg, "Invalid User");
        return 0;
    }
    state->authenticated = 0;
    strncpy(state->user, input[1], MAX_USER_SIZE);
    strcat(state->msg, "User OK");
    return 1;
}

void createFolder(char *pwd)
{
    struct stat st = {0};
    // https://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
    if (stat(pwd, &st) == -1)
        mkdir(pwd, 0700);
}
// returns 1 if succesful
// returns 0 otherwise
int pass(char **input, int length, struct State *state)
{

    // If the number of arguments is not 2 (PASS + "<password>"),
    // or if the user does not exist
    // Then the argument fails
    if (length != 2) // TODO: Create separate error messages
    {
        strcat(state->msg, "Invalid Number of Parameters");
        return 0;
    }
    if (state->user == NULL || state->authenticated == 1)
    {
        // 503 Bad sequence of commands
        strcat(state->msg, "503 Bad sequence of commands");
        return 0;
    }
    if (!verifyUserPassword(state->user, input[1]))
    {
        // Wrong password
        strcat(state->msg, "Wrong password");
        return 0;
    }
    state->authenticated = 1;
    strcpy(state->pwd, state->user);
    strcat(state->pwd, "/");
    createFolder(state->pwd);
    strcat(state->msg, "Logged In!");

    return 1;
}

int list(char **input, int length, struct State *state)
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
            strcat(state->msg, dir->d_name);
            strcat(state->msg, "\n");
        }
        closedir(d);
    }
    else
    {
        // TODO: Figure this out
        // directory does not exist (if errno == ENOENT) with #include <errno.h>
        return 0;
    }
    return 1;
}

int pwd(char **input, int length, struct State *state)
{
    strcat(state->msg, state->pwd);
    return 1;
}

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
        strcat(state->msg, "Invalid number of arguments");
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
        strcat(state->msg, "Folder does not exist!\n");
        return 0;
    }
    bzero(state->pwd, sizeof(state->pwd));
    strcpy(state->pwd, newPWD);
    return 1;
}

int openDataConnection(char *address, int length, struct State *state)
{
    int ftp_connection = socket(AF_INET, SOCK_STREAM, 0);
    if (ftp_connection < 0)
    {
        perror("socket:");
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
    // Get Client address
    // 192,168,0,1,200,100
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
    if (connect(ftp_connection, (struct sockaddr *)&ftp_connection_client_addr, sizeof(ftp_connection_client_addr)) < 0)
    {
        perror("connect");
        return 0;
    }
    char PORTOK[] = "PORT OK";
    printf("Sending: %s\nLength: %ld\n", PORTOK, strlen(PORTOK));
    send(ftp_connection, PORTOK, strlen(PORTOK), 0);
    close(ftp_connection);
    // strncpy(state->msg, PORTOK, strlen(PORTOK));
    // strncpy(state->user, input[1], MAX_USER_SIZE);

    return 1;
}

// void selectCommand(char** input, int length, struct State* state){
void selectCommand(char buffer[], int buffer_size, struct State *state)
{
    int length = 0;
    printf("Buffer: %s\n", buffer);
    char **input = splitString(buffer, buffer_size, &length);
    printf("Length: %d\n", length);
    if (length == 0)
        return;

    bzero(state->msg, MAX_MESSAGE_SIZE * sizeof(char));
    char *command = input[0];
    if (command == NULL)
    {
        // handle null
        printf("Error: Null Command \n");
        return;
    }
    else if (strcmp(command, "USER") == 0)
    {
        user(input, length, state);
    }
    else if (strcmp(command, "PASS") == 0)
    {
        pass(input, length, state);
    }
    else if (!(state->authenticated))
    {
        // Not authenticated
        strcat(state->msg, "User not authenticated!");
    }
    else if (strcmp(command, "PORT") == 0)
    {
        if (!openDataConnection(input[1], length, state))
        {
            strcpy(state->msg, "Problem occured!");
        }
    }
    // else if (strcmp(command, "STOR") == 0)
    // {
    // }
    // else if (strcmp(command, "RETR") == 0)
    // {
    // }
    else if (strcmp(command, "LIST") == 0)
    {
        list(input, length, state);
    }
    else if (strcmp(command, "CWD") == 0)
    {
        cwd(input, length, state);
    }
    else if (strcmp(command, "PWD") == 0)
    {
        pwd(input, length, state);
    }
    else
    {
        strcpy(state->msg, "No such command");
    }
}