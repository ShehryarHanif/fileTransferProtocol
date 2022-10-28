#define MAX_MESSAGE_SIZE 5000
#define MAX_LINUX_DIR_SIZE 4097
#define MAX_USER_SIZE 33
#define MAX_IPADDRSTR_SIZE 15

// Each state represents details for each user, especially with regards to their control channel
struct State {
    char pwd[MAX_LINUX_DIR_SIZE];
    char user[MAX_USER_SIZE];
    char msg[MAX_MESSAGE_SIZE];
    int authenticated;
    int client_sd;
    int ftp_port;
    char ftp_ip_addr[MAX_IPADDRSTR_SIZE];
    int ftp_client_connection;
};

struct State state[MAX_CLIENTS]; // This array stores the clients' states based on a globally defined maximum number of clients

void resetState(struct State* state){ // Reset the state for an individual user (e.g., if they disconnect)
    bzero(state->pwd, MAX_LINUX_DIR_SIZE*sizeof(char));
    bzero(state->user, MAX_USER_SIZE*sizeof(char));
    bzero(state->msg, MAX_MESSAGE_SIZE*sizeof(char));
    bzero(state->ftp_ip_addr, MAX_IPADDRSTR_SIZE*sizeof(char));
    state->authenticated = 0;
    state->client_sd = -1;
    state->ftp_client_connection = -1;
    state->ftp_port = -1;
}