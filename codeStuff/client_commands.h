// check if local command
int isLocalCommand(char* input){
    return input[0] == '!';
}

// returns 1 if command is local (!PWD) or (!CWD)
// TODO: Add note for this design choice
// ----- Directory manipulation without login
int selectCommand(char** input, int length){
    char* command = input[0];
    if (strcmp(command, "!CWD") == 0){
        // cwd(input, length, state);
    } else if (strcmp(command, "!PWD") == 0){
        // pwd(input, length, state);
    } else {
        
    }
}