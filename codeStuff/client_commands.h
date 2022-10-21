#include <dirent.h>

// check if local command
int isLocalCommand(char *input)
{
    return input[0] == '!';
}

int pwd(struct State *state)
{
    printf("%s\n", state->pwd);
    return 1;
}

// if the argument of cwd is "..", we want to manually edit the pwd string
int goBackFolder(struct State *state)
{
    length = strlen(state->pwd);
    int i = length-1;
    while (i >= 0) {
        if ()
    }
}

int cwd(char **input, int length, struct State *state)
{
    if (length != 2)
    {
        printf("Invalid number of arguments");
        return 0;
    }
    char *newDir = input[1];
    char *newPWD = malloc((strlen(state->pwd) + strlen(newDir) + 1) * sizeof(char));
    strcpy(newPWD, state->pwd);
    strcat(newPWD, newDir);
    strcat(newPWD, "/");
    // Check if newPWD is a valid dir
    DIR *d;
    d = opendir(newPWD);
    if (!d)
    {
        // given folder does not exist
        // TODO: Proper error msg
        printf("Folder `%s` does not exist!", newPWD);
        return 0;
    }
    bzero(state->pwd, sizeof(state->pwd));
    strcpy(state->pwd, newPWD);
    free(newPWD);
    return 1;
}

int list(struct State *state)
{
    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;
    d = opendir(state->pwd);

    // bzero(state->msg);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    else
    {
        // TODO: Figure this out
        // directory does not exist (if errno == ENOENT) with #include <errno.h>
        printf("Folder does not exist");
        return 0;
    }
    return 1;
}

// returns 1 if command is local (!PWD) or (!CWD)
// TODO: Add note for this design choice
// ----- Directory manipulation without login
int selectCommand(char **input, int length, struct State *state)
{
    char *command = input[0];
    if (strcmp(command, "!CWD") == 0)
    {
        cwd(input, length, state);
    }
    else if (strcmp(command, "!PWD") == 0)
    {
        pwd(state);
    }
    else if (strcmp(command, "!LIST") == 0)
    {
        list(state);
    }
    else
    {
    }
}