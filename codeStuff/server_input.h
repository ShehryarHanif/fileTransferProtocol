#define MAX_ARGUMENTS 5

char** splitString(char buffer[], int buffer_size, int* length){ // Split a string into an array of strings, with " " as the delimiter
    // Size of input array is "MAX_ARGUMENTS + 1" because the last element is "NULL"

    char buffer_copy[buffer_size];

    strcpy(buffer_copy, buffer);

    char** input = malloc((MAX_ARGUMENTS + 1) * sizeof(char*));

    for (int j = 0; j < MAX_ARGUMENTS + 1; j++){
        input[j] = NULL;
    }

    char* ptr = strtok(buffer_copy," ");

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

void freeInputMemory(char* input[MAX_ARGUMENTS + 1]){ // Delete the input array at the end of the running
    for (int i = 0; i < MAX_ARGUMENTS + 1; i++){
        free(input[i]);
    }

    free(input);
}