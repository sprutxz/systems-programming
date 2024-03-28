#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <glob.h>

#define BUFFER_SIZE 128

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

void freeTokens(char** tokens, int numArgs)
{
    for (int i = 0; i < numArgs; i++){
        free(tokens[i]);
    }
    free(tokens);
}

void cd(char* path)
{
    if (chdir(path) != 0){
        perror("No such file or directory\n");
    }
}

void pwd()
{
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL){
        perror("Error getting current working directory\n");
    }
    printf("%s\n", cwd);
    free(cwd);
}

char* searchExec(char* executable){
    char** path = (char**) malloc(3 * sizeof(char*));
    if (path == NULL){
        perror("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }

    path[0] = (char*) malloc((15 + strlen(executable) +1) * sizeof(char));
    memcpy(path[0], "/usr/local/bin/", 15);
    memcpy(path[0] + 15, executable, strlen(executable));
    path[0][15 + strlen(executable)] = '\0';

    path[1] = (char*) malloc((9 + strlen(executable) +1) * sizeof(char));
    memcpy(path[1], "/usr/bin/", 9);
    memcpy(path[1] + 9, executable, strlen(executable));
    path[1][9 + strlen(executable)] = '\0';

    path[2] = (char*) malloc((5 + strlen(executable) +1) * sizeof(char));
    memcpy(path[2], "/bin/", 5);
    memcpy(path[2] + 5, executable, strlen(executable));
    path[2][5 + strlen(executable)] = '\0';

    for (int i = 0; i < 3; i++){
        if(access(path[i], F_OK) == 0){
            char* pathToExec = strdup(path[i]);
            for (int j = 0; j < 3; j++){
                free(path[j]);
            }
            free(path);
            return pathToExec;
        }
    }
    return NULL;
}


void parseCommand(char* command)
{
    char* inputFile = NULL;
    char* outputFile = NULL;

    for (int i = 0; i < strlen(command); i++){
        if (command[i] == '<'){
            inputFile = redirection(command, i);
        }else if (command[i] == '>'){
            outputFile = redirection(command, i);
        }
    }

    int numArgs = 0;
    char delim[]=" \t\r\n\v\f\0";
    for (int i = 0; i < strlen(command)+1; i++){
        if (strchr(delim, command[i]) && !isspace(command[i-1])){
            numArgs++;
        }
    }

    char** tokens = (char**) malloc((numArgs + 1) * sizeof(char*));
    char* token = strtok(command, " ");
    int i = 0;
    while (token != NULL){
        if (strchr(token, '*')){

            glob_t glob_result;
            
            glob(token, GLOB_TILDE, NULL, &glob_result);

            if (glob_result.gl_pathc == 0){
                tokens[i] = strdup(token);
                token = strtok(NULL, " ");
                i++;
                globfree(&glob_result);
                continue;
            }

            numArgs += glob_result.gl_pathc-1;

            tokens = (char**) realloc(tokens, (numArgs+1) * sizeof(char*));
            for (unsigned int j = 0; j < glob_result.gl_pathc; j++){
                tokens[i] = strdup(glob_result.gl_pathv[j]);
                i++;
            }

            globfree(&glob_result);
            token = strtok(NULL, " ");
            continue;
        }

        tokens[i] = strdup(token);
        token = strtok(NULL, " ");
        i++;
    }
    tokens[numArgs] = NULL;

    if (strcmp(tokens[0], "cd") == 0){
        if (numArgs != 2){
            perror("Invalid number of arguments\n");
            freeTokens(tokens, numArgs);
            return;
        }
        cd(tokens[1]);
        freeTokens(tokens, numArgs);
        return;
    }

    if (strcmp(tokens[0], "pwd") == 0){
        if (numArgs != 1){
            perror("Invalid number of arguments\n");
            freeTokens(tokens, numArgs);
            return;
        }

        pwd();
        freeTokens(tokens, numArgs);
        return;
    }

    if (strcmp(tokens[0], "which") == 0){
        if (numArgs != 2){
            perror("Invalid number of arguments\n");
            freeTokens(tokens, numArgs);
            return;
        }
        char* pathToExec = searchExec(tokens[1]);

        if (pathToExec != NULL){
            printf("%s\n", pathToExec);
        }

        free(pathToExec);
        freeTokens(tokens, numArgs);
        return;
    }

    char* exec = NULL;
    if (strchr(tokens[0], '/')){
        exec = strdup(tokens[0]);
    }else{
        exec = searchExec(tokens[0]);
        if (exec == NULL){
            perror("Error finding executable\n");
            freeTokens(tokens, numArgs);
            return;
        }
    }


    pid_t pid = fork();
    if (pid < 0){
        perror("Error forking\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0){
        if (inputFile != NULL){
            int input_fd = open(inputFile, O_RDONLY);
            if (input_fd == -1){
                perror("Error opening input file\n");
                exit(EXIT_FAILURE);
            }

            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
            free(inputFile);
        }

        if (outputFile != NULL){
            int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (output_fd == -1){
                perror("Error opening output file\n");
                exit(EXIT_FAILURE);
            }

            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
            free(outputFile);
        }

        execvp(exec, tokens);
        free(exec);
    }

    int status;
    wait(&status);

    // if (!WIFEXITED(status)){
    //     printf("Child process exited with status %d\n", WEXITSTATUS(status));
    // }

    // child process done

    freeTokens(tokens, numArgs);
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