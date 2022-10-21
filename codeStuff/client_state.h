#define MAX_LINUX_DIR_SIZE 4097

struct State {
    char pwd[MAX_LINUX_DIR_SIZE]; // present working directory
};

int initializePWD(struct State* state){

    char* result = getcwd(state->pwd, (MAX_LINUX_DIR_SIZE-2)*sizeof(char));
    strcat(state->pwd, "/");
    if (!result) return 0;
    return 1;
}