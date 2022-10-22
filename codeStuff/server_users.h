// Functions
int readUsers();
int getNumLines(FILE *);
int verifyUserExists(char*);

#define FILE_BUFFER_SIZE 256



// Function and Variable Declrations
struct User{
    char user[MAX_USER_SIZE];
    char password[MAX_USER_SIZE];
};

struct User* users; // Array for the users
int numberUsers; // int to keep track of size of array


// returns 1 if successful
// returns 0 if not successful

// TODO: Create user folders
int readUsers(){
    if(DEBUG) printf("Reading...\n");
    FILE *fp;
    char user_buff[FILE_BUFFER_SIZE];
    char pass_buff[FILE_BUFFER_SIZE];
    if ((fp = fopen("../users.txt", "r")) == NULL){
        printf("Could not open file/Not Found\n");
        return 0;
    }
    // Get number of users to assign size to the users array
    numberUsers = getNumLines(fp);
    users = malloc(numberUsers*sizeof(struct User));
    fseek(fp, 0, SEEK_SET);

    if(DEBUG) printf("====Reading Users====\n");

    // Read through the file using numberUsers
    for(int i = 0; i < numberUsers; i++) {
        int scanned = fscanf(fp, "%255s %255s\n", user_buff, pass_buff);
        if (scanned < 2){
            free(users);
            printf("Invalid Format of `users.txt`");
            return 0; // Error in file reading or Error in users.txt
        }
        if(DEBUG) printf("Read User: %s, Pass: %s\n", user_buff, pass_buff);
        if (strlen(user_buff) >= MAX_USER_SIZE-1){
            printf("Invalid user name length. Max Length: %d", MAX_USER_SIZE-1);
            return 0;
        }
        if (strlen(pass_buff) >= MAX_USER_SIZE-1){
            printf("Invalid password length. Max Length: %d", MAX_USER_SIZE-1);
            return 0;
        }
        strncpy(users[i].user, user_buff, MAX_USER_SIZE);
        strncpy(users[i].password, pass_buff, MAX_USER_SIZE);
    }
    if(DEBUG) printf("===================\nConfirming Read: \n");

    if (DEBUG)
        for (int i = 0; i < numberUsers; i++)
            printf("User %d: %s, Pass: %s, Exist:%d\n", i, users[i].user, users[i].password, verifyUserExists(users[i].user));

    if(DEBUG) printf("===================\n");
    
    return 1;
}

// Get number of lines in users.txt
// Assume users.txt is not empty
int getNumLines(FILE *fp){
    int count = 1;
    for (char c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n'){
            count++;
        }
    }
    return count;
}

// Check if user exists, and no other client is connected with that user names
// returns 1 if exists
// 0 otherwise
int verifyUserExists(char* user){
    // Checking if username exists
    for (int i = 0; i < numberUsers; i++){
        printf("User checking...%s\nWith %s\nPASS %s\n", users[i].user, user, users[i].password);
        if (strcmp(users[i].user, user)==0)
            return 1;
    }
        
            
    return 0;
}

// Check if the username and password match
int verifyUserPassword(char* user, char* password){
    // Checking if another state is logged in using that username
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (strncmp(state[i].user, user, MAX_USER_SIZE-1) == 0 && state[i].authenticated == 1)
            return 0;
    // Check if the username and password match
    for (int i = 0; i < numberUsers; i++)
        if (strcmp(users[i].user, user)==0)
            return strcmp(users[i].password, password) == 0 ? 1 : 0;
    return 0;
}