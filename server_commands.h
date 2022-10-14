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

void selectCommand(char** input, int length){
    char* command = input[0];
    if (command == NULL) {
        // handle null
        printf("Error: Null Command \n");
        return;
    }
    if (strcmp(command, "USER") == 0){

    } else if (strcmp(command, "PASS") == 0){

    } else if (strcmp(command, "PORT") == 0){

    } else if (strcmp(command, "STOR") == 0){

    } else if (strcmp(command, "RETR") == 0){

    } else if (strcmp(command, "LIST") == 0){

    } else if (strcmp(command, "CWD") == 0){

    } else if (strcmp(command, "PWD") == 0){
        printf("PWDing\n");
    } else if (command[0] == '!') // CLient commands
    { 
        if (strcmp(command, "!PWD")){
            
        } else if (strcmp(command, "!CWD")){

        }
    } else {

    }
}

int user(char** input, int length){
    
    if (length != 2) // checking number of arguments:
    {
        return 0;
    }
    
}

// int verifyLength(int length, int expected_length){
//     return length == expected_length;
// }