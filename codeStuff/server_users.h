#define FILE_BUFFER_SIZE 256

// Functions

int readUsers();
int getNumLines(FILE *);
int verifyUserExists(char*);

// Function and Variable Declarations
struct User{
    char user[MAX_USER_SIZE];
    char password[MAX_USER_SIZE];
};

struct User* users; // Array for the users
int numberUsers; // Integer to keep track of the size of array

int readUsers(){ // Read the users from the text file and store information about them in an array of "stuct"s
    if(DEBUG) printf("Reading...\n");

    FILE *fp;

    char user_buff[FILE_BUFFER_SIZE];
    char pass_buff[FILE_BUFFER_SIZE];

    if ((fp = fopen("../users.txt", "r")) == NULL){
        printf("The specific file could not be opened\n");

        return 0;
    }

    numberUsers = getNumLines(fp); // Get the number of users to assign size to the users array

    users = malloc(numberUsers*sizeof(struct User));

    fseek(fp, 0, SEEK_SET);

    if(DEBUG) printf("====Reading Users====\n");

    // Read through the file using the number of users

    for(int i = 0; i < numberUsers; i++){
        int scanned = fscanf(fp, "%255s %255s\n", user_buff, pass_buff);

        if (scanned < 2){
            free(users);

            printf("Invalid Format of `users.txt`");

            return 0; // Error in file reading or error in "users.txt"
        }

        if(DEBUG) printf("Read User: %s, Pass: %s\n", user_buff, pass_buff);

        if (strlen(user_buff) >= MAX_USER_SIZE - 1){
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

    if(DEBUG) printf("===================\nConfirming Read\n");

    if (DEBUG){
        for (int i = 0; i < numberUsers; i++)
            printf("User %d: %s, Pass: %s, Exist:%d\n", i, users[i].user, users[i].password, verifyUserExists(users[i].user));
    }

    if(DEBUG) printf("===================\n");
    
    return 1;
}

int getNumLines(FILE *fp){ // Get the number of lines in "users.txt" after assuming it is non-empty
    int count = 1;

    for (char c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n'){
            count++;
        }
    }

    return count;
}

int verifyUserExists(char* user){ // Check if the user exists, and no other client is connected with that username when trying to log in
    for (int i = 0; i < numberUsers; i++){
        printf("User checking...%s\nWith %s\nPASS %s\n", users[i].user, user, users[i].password);

        if (strcmp(users[i].user, user)==0)
            return 1;
    }
               
    return 0;
}

int verifyUserPassword(char* user, char* password){ // Check if the username and password match when trying to log in
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (strncmp(state[i].user, user, MAX_USER_SIZE-1) == 0 && state[i].authenticated == 1)
            return 0;

    // Check if the username and password match

    for (int i = 0; i < numberUsers; i++)
        if (strcmp(users[i].user, user)==0)
            return strcmp(users[i].password, password) == 0 ? 1 : 0;
            
    return 0;
}