#define MAX_LINUX_DIR_SIZE 4097
#define MAX_IPADDRSTR_SIZE 16

struct State {
    char pwd[MAX_LINUX_DIR_SIZE]; // present working directory
    char ipaddr[MAX_IPADDRSTR_SIZE]; // remember to use inet_ntoa
    int port;
    int server_sd;
    int displacement;
};

int initializePWD(struct State* state){

    char* result = getcwd(state->pwd, (MAX_LINUX_DIR_SIZE-2)*sizeof(char));
    strcat(state->pwd, "/");
    if (!result) return 0;
    return 1;
}