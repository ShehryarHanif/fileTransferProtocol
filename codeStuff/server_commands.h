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

// returns 1 if succesful
// returns 0 otherwise
int user(char** input, int length, struct State* state){

    // If the number of arguments is not 2 (USER + "<username>"),
    // or if the user does not exist
    // Then the argument fails
    if (length != 2)
    {
        // Error msg: Invalid Number of Parameters
        return 0;
    }
    if (!verifyUserExists(input[1]))
    {
        // Error msg: Invalid User
        return 0;
    }
    state->authenticated = 0;
    strncpy(state->user, input[1], MAX_USER_SIZE);
    return 1;
}

// returns 1 if succesful
// returns 0 otherwise
int pass(char** input, int length, struct State* state)
{

    // If the number of arguments is not 2 (PASS + "<password>"),
    // or if the user does not exist
    // Then the argument fails
    if (length != 2) // TODO: Create separate error messages
    {
        // Error msg: Invalid length
        return 0;
    }
    if (state->user == NULL || state->authenticated == 1) {
        // 503 Bad sequence of commands
        return 0;
    }
    if (!verifyUserPassword(state->user, input[1])){
        // Wrong password
        return 0;
    }
    state->authenticated = 1;
    strcpy(state->pwd, state->user);
    strcat(state->pwd, "/");
    return 1;
}

int list(char** input, int length, struct State* state)
{

    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;
    d = opendir(state->pwd);

    // bzero(state->msg);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            strcat(state->msg, dir->d_name);
            strcat(state->msg, "\n");
        }
        closedir(d);
    } else {
        // TODO: Figure this out
        // directory does not exist (if errno == ENOENT) with #include <errno.h>
        return 0;
    }
    return 1;

}

int pwd(char** input, int length, struct State* state)
{
    strcat(state->msg, state->pwd);
    return 1;
}

int cwd(char** input, int length, struct State* state)
{
    char* newDir = input[1];
    char* newPWD = malloc((strlen(state->pwd) + strlen(newDir) + 1) * sizeof(char));
    strcpy(newPWD, state->pwd);
    strcat(newPWD, newDir);
    strcat(newPWD, "/");
    // Check if newPWD is a valid dir
    DIR *d;
    d = opendir(newPWD);
    if (!d) {
        // given folder does not exist
        return 0;
    }
    bzero(state->pwd, sizeof(state->pwd));
    strcpy(state->pwd, newPWD);
    return 1;
}

void selectCommand(char** input, int length, struct State* state){
    bzero(state->msg, MAX_MESSAGE_SIZE*sizeof(char));
    char* command = input[0];
    if (command == NULL) {
        // handle null
        printf("Error: Null Command \n");
        return;
    }
    if (strcmp(command, "USER") == 0){
        user(input, length, state);
    } else if (strcmp(command, "PASS") == 0){
        pass(input, length, state);
    } else if (!(state->authenticated)) {
        // Not authenticated
    } else if (strcmp(command, "PORT") == 0){

    } else if (strcmp(command, "STOR") == 0){

    } else if (strcmp(command, "RETR") == 0){

    } else if (strcmp(command, "LIST") == 0){
        list(input, length, state);
    } else if (strcmp(command, "CWD") == 0){
        // cwd(input, length, state);
    } else if (strcmp(command, "PWD") == 0){
        pwd(input, length, state);
    // } else if (command[0] == '!') // CLient commands
    // { 
    //     if (strcmp(command, "!PWD")){
            
    //     } else if (strcmp(command, "!CWD")){

    //     }
    } else {
        
    }
}