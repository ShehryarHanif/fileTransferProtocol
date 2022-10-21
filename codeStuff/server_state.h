#define MAX_MESSAGE_SIZE 4097
#define MAX_LINUX_DIR_SIZE 4097
#define MAX_USER_SIZE 33

struct State {
    char pwd[MAX_LINUX_DIR_SIZE]; // present working directory
    char user[MAX_USER_SIZE];
    char msg[MAX_MESSAGE_SIZE];
    int authenticated; // 1 if authenticated; 0 otherwise
};