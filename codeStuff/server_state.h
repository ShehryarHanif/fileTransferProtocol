#define MAX_MESSAGE_SIZE 4097
#define MAX_LINUX_DIR_SIZE 4097
#define MAX_USER_SIZE 33
#define MAX_IPADDRSTR_SIZE 15

struct State {
    char pwd[MAX_LINUX_DIR_SIZE]; // present working directory
    char user[MAX_USER_SIZE];
    char msg[MAX_MESSAGE_SIZE];
    int authenticated; // 1 if authenticated; 0 otherwise
    int client_sd;
};

struct State state[MAX_CLIENTS];

void resetState(struct State* state)
{
    bzero(state->pwd, MAX_LINUX_DIR_SIZE*sizeof(char));
    bzero(state->user, MAX_USER_SIZE*sizeof(char));
    bzero(state->msg, MAX_MESSAGE_SIZE*sizeof(char));
    state->authenticated = 0;
}

// initializeStates(){
//     for
// }