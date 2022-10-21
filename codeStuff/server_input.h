#define MAX_ARGUMENTS 5


// "PORT 21,21,21,12,21,21"
// split = ["PORT", "21", "21", "21", "12", "21", "21", NULL];

char** splitString(char buffer[], int* length){

    // Size of input array is MAX_ARGUMENTS+1
    char** input = malloc((MAX_ARGUMENTS+1)*sizeof(char*));

    for (int i = 0; i < MAX_ARGUMENTS+1; i++){
        input[i] = NULL;
    }

    printf("Buffer: %s\n", buffer);
    char* ptr = strtok(buffer," ");
    int i = 0;
    
    while (ptr != NULL)
	{
        input[i] = (char *)malloc((strlen(ptr)+1)*sizeof(char));
        strcpy(input[i], ptr);
        ptr = strtok(NULL, " ");
        i++;
    }
    *length = i;

    
    return input;
}

void freeInputMemory(char* input[MAX_ARGUMENTS+1]){
    for (int i = 0; i < MAX_ARGUMENTS+1; i++){
        free(input[i]);
    }
    free(input);
}