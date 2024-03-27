#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4


char* redirection(char* command, int index)
{
    char* startPtr = command + index + 1;
    while (*startPtr == ' '){
        startPtr++;
    }

    char* endPtr = startPtr;
    while (*endPtr != ' ' && *endPtr != '\0'){
        endPtr++;
    }

    char* file = (char*) malloc(endPtr - startPtr + 1);
    memcpy(file, startPtr, endPtr - startPtr);
    file[endPtr - startPtr] = '\0';

    memmove(command + index, endPtr, strlen(endPtr) + 1);

    return file;
}

void parseCommand(char* command)
{
    char* inputFile = NULL;
    char* outputFile = NULL;

    for (int i = 0; i < strlen(command); i++){
        if (command[i] == '<'){
            inputFile = redirection(command, i); // rememeber to free this
        }else if (command[i] == '>'){
            outputFile = redirection(command, i); // rememeber to free this
        }
    }

    int numArgs = 0;
    char delim[]=" \t\r\n\v\f\0";
    for (int i = 0; i < strlen(command)+1; i++){
        if (strchr(delim, command[i]) && isalnum(command[i-1])){
            numArgs++;
        }
    }

    char* tokens[numArgs+1];
    char* token = strtok(command, " ");
    int i = 0;
    while (token != NULL){
        tokens[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    tokens[numArgs] = NULL;

    pid_t pid = fork();
    if (pid < 0){
        perror("Error forking\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0){
        execvp(tokens[0], tokens);
    }
    int status;
    wait(&status);
    if (WIFEXITED(status)){
        printf("Child process exited with status %d\n", WEXITSTATUS(status));
    }
    printf("Child process has finished executing.\n");
}

void program(int fd)
{
    if (isatty(fd)){
        // run interactive mode
        char buffer[BUFFER_SIZE];
        char* string = NULL;
        int bytes = 0;
        ssize_t bytesRead;

        write(STDOUT_FILENO, "Initializing shell...\n", 23);
        write(STDOUT_FILENO, "sh> ", 4);


        while (1) {
            if ((bytesRead = read(STDIN_FILENO, buffer, BUFFER_SIZE)) == -1) {
                perror("Error reading from stdin");
                exit(EXIT_FAILURE);
            }
            
            
            if ((string = (char*) realloc(string, bytes + bytesRead)) == NULL){
                perror("Error allocating memory\n");
                exit(EXIT_FAILURE);
            }

            memcpy(string + bytes, buffer, bytesRead);
            bytes += bytesRead;

            if  (string[bytes - 1] == '\n'){
                if (bytes == 1){
                    bytes = 0;
                    free(string);
                    string = NULL;
                    write(STDOUT_FILENO, "sh> ", 4);
                    continue;
                }
                string[bytes-1] = '\0';

                if (strcmp(string, "exit") == 0){
                    // Make exit print all argument it recieves then exit

                    write(STDOUT_FILENO, "Exiting shell...\n", 18);
                    free(string);
                    break;
                }

                parseCommand(string);
                bytes = 0;
                free(string);
                string = NULL;
                write(STDOUT_FILENO, "sh> ", 4);
            }
        }
    }else{
        
        // run batch mode
    }

}

int main(int argc, char* argv[])
{
    if (argc > 2){
        perror("Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    
    if (argc == 2){
        // Batch mode
        int batch_fd;
        if ((batch_fd = open(argv[1], O_RDONLY)) == -1){
            perror("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        program(batch_fd);

    }else{
        // Interactive mode
        program(STDIN_FILENO);
    }
}