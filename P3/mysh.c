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

char* ExtractRedirectionFile(char* command, int index)
{
    char* startPtr = command + index + 1;
    while (*startPtr == ' '){
        startPtr++;
    }

    char* endPtr = startPtr;
    while (*endPtr != ' ' && *endPtr != '\0' && *endPtr != '|' && *endPtr != '<' && *endPtr != '>'){
        endPtr++;
    }

    char* file = (char*) malloc(endPtr - startPtr + 1);
    memcpy(file, startPtr, endPtr - startPtr);
    file[endPtr - startPtr] = '\0';

    memmove(command + index, endPtr, strlen(endPtr) + 1);

    return file;
}

char* BareNameSearch (char* executable)
{
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

char** tokenize (char* command, int numArgs)
{
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
            if (tokens == NULL){
                perror("Error allocating memory\n");
                exit(EXIT_FAILURE);
            }

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

    return tokens;
}

void freeTokens (char** tokens, int numArgs)
{
    for (int i = 0; i < numArgs; i++){
        free(tokens[i]);
    }
    free(tokens);
}

int isBuiltin (char* cmd) 
{
    if (strcmp (cmd, "cd") == 0) {
        return 1;
    }

    if (strcmp (cmd, "pwd") == 0) {
        return 2;
    }

    if (strcmp (cmd, "which") == 0){
        return 3;
    }

    return 0;

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
    write(STDOUT_FILENO, cwd, strlen(cwd));
    write(STDOUT_FILENO, "\n", 1);
    free(cwd);
}

int BuiltInFunctions (char** args, int numArgs, int index)
{
    if (index == 1){
        if (numArgs != 2){
            perror("Invalid number of arguments\n");
            return -1;
        }
        cd(args[1]);
        return 1;
    }

    if (index == 2){
        if (numArgs != 1){
            perror("Invalid number of arguments\n");
            return -1;
        }
        pwd();
        return 1;
    }

    if (index == 3){
        if (numArgs != 2){
            perror("Invalid number of arguments\n");
            return -1;
        }
        char* pathToExec = BareNameSearch(args[1]);

        if (pathToExec != NULL){
           write(STDOUT_FILENO, pathToExec, strlen(pathToExec));
           write(STDOUT_FILENO, "\n", 1);
        }

        free(pathToExec);
        return 1;
    }

    return 0;
}

void openAndRedirect(char* file, int input_or_output)
{
    int fd;
    if (input_or_output == 1){
        fd = open(file, O_RDONLY);
        if (fd == -1){
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }else{
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd == -1){
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

char* findExecutable(char* executable)
{
    char* path = NULL;
    if (strchr(executable, '/')){
        path = strdup(executable);
    }else{
        path = BareNameSearch(executable);
        if (path == NULL){
            perror("Error finding executable");
            return NULL;
        }
    }
    return path;
}

void parseCommand(char* command)
{
    char* inputFile = NULL;
    char* outputFile = NULL;
    char* pipeInputFile = NULL;
    char* pipeOutputFile = NULL;

    int pipe_flag = 0; // flag to indicate if there is a pipe in the command

    /* Loop to check for any redirection and store them appropriately */
    for (int i = 0; i < (int) strlen(command); i++) {
        if (command[i] == '|') {
            pipe_flag = 1;
        }

        char* redirectionFile = NULL;
        if (command[i] == '<') {
            redirectionFile = ExtractRedirectionFile(command, i);
            i--;
            if (pipe_flag == 0){
                inputFile = redirectionFile;
            }else{ 
                pipeInputFile = redirectionFile;
            }
        }

        if (command[i] == '>') {
            redirectionFile = ExtractRedirectionFile(command, i);
            i--;
            if (pipe_flag == 0) {
                outputFile = redirectionFile;
            }else {
                pipeOutputFile = redirectionFile;
            }
        }
    }

    int numArgs[2] = {0, 0};
    char delim[]=" \t\r\n\v\f\0";
    int flag = 0;

    /* Loop to count the number of arguments in the command */
    int len = strlen(command); // we store length as we will be add a null terminator in the commadn string incase of a pipe
    for (int i = 0; i < len+1; i++){
        if (command[i] == '|'){

            if (!strchr(delim, command[i-1])){ /* we increase total args for first command by 1 incase ...*/ 
                numArgs[0]++;                  /* ... there is no space delimiter before the pipe */
            }

            command[i] = '\0'; // add a null terminator to the command string to separate the two commands
            flag = i; // flag to indicate that we have encountered a pipe, also acts as an index of the pipe

            continue;
        }

        if (strchr(delim, command[i]) && !strchr(delim, command[i-1])){
            if (!flag) numArgs[0]++; // increase the number of arguments for the first command until pipe is encountered
            else numArgs[1]++; // increase the number of arguments for the second command after pipe is encountered
        }
    }

    int exit_status; // variable to store the exit status of the child process

    if (pipe_flag == 0){

        char** tokens = tokenize(command, numArgs[0]); // we tokenize the command string to get the arguments

        /* We check if the command is a builtin command.
           If builtin then execute in parent else create
           a child process and execute the command */
        int i;
        if ((i = isBuiltin(tokens[0])) != 0){

            BuiltInFunctions(tokens, numArgs[0], i);

        } else {

            char* pathToExec = findExecutable(tokens[0]); // Find the path to the executable file

            if (pathToExec == NULL){
                perror("Error finding executable");
                return;
            }

            pid_t pid;
            if ((pid = fork()) < 0){
                perror("Error forking");
                exit(EXIT_FAILURE);
            }

            if (pid == 0){
                if (inputFile != NULL){
                    openAndRedirect(inputFile, 1);
                }
                if (outputFile != NULL){
                    openAndRedirect(outputFile, 0);
                }

                execv(pathToExec, tokens);
                perror("Error executing command");
                exit(EXIT_FAILURE);
            } else {
                free(pathToExec);
            }
        }

        wait(&exit_status); // wait for the child process to finish and store the exit status
        freeTokens(tokens, numArgs[0]); // free the tokens

    } else {

        char** tokens = tokenize(command, numArgs[0]);
        char** pipetokens = tokenize(command + flag + 1, numArgs[1]);
        int i;
        int pipefd[2];

        if (pipe(pipefd) == -1){
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        if ((i = isBuiltin(tokens[0])) != 0){
            int save_stdout = dup(STDOUT_FILENO);

            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
            close(pipefd[1]); // Close the write end of the pipe

            BuiltInFunctions(tokens, numArgs[0], i);

            dup2(save_stdout, STDOUT_FILENO);
            close(save_stdout);

        } else {

            char* pathToExec = findExecutable(tokens[0]);
            if (pathToExec == NULL){
                perror("Error finding executable");
                return;
            }

            pid_t pid;
            if ((pid = fork()) < 0){
                perror("Error forking");
                exit(EXIT_FAILURE);
            }

            if (pid == 0){
                
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);

                if (inputFile != NULL){
                    openAndRedirect(inputFile, 1);
                }
                if (outputFile != NULL){
                    openAndRedirect(outputFile, 0);
                }

                execv(pathToExec, tokens);
                perror("Error executing command");
                exit(EXIT_FAILURE);
            } else {
                close(pipefd[1]);
                free(pathToExec);
            } 
        }

        /* SECOND CHILD PROCESS */
        if ((i = isBuiltin(pipetokens[0])) != 0){
            int save_stdin = dup(STDIN_FILENO);
            
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            BuiltInFunctions(pipetokens, numArgs[1], i);

            dup2(save_stdin, STDIN_FILENO);
            close(save_stdin);

        } else {

            char* pathToExec = findExecutable(pipetokens[0]);
            if (pathToExec == NULL){
                perror("Error finding executable");
                return;
            }

            pid_t pid;
            if ((pid = fork()) < 0){
                perror("Error forking");
                exit(EXIT_FAILURE);
            }

            if (pid == 0){
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                if (pipeOutputFile != NULL){
                    openAndRedirect(pipeOutputFile, 0);
                }
                if (pipeInputFile != NULL){
                    openAndRedirect(pipeInputFile, 1);
                }

                execv(pathToExec, pipetokens);
                perror("Error executing command");
                exit(EXIT_FAILURE);
            } else {
                close(pipefd[0]);
                free(pathToExec);
            }
        }
        wait(&exit_status);
        wait(&exit_status);
        freeTokens(tokens, numArgs[0]);
        freeTokens(pipetokens, numArgs[1]);
    }

    if (inputFile) free(inputFile);
    if (outputFile) free(outputFile);
    if (pipeInputFile) free(pipeInputFile);
    if (pipeOutputFile) free(pipeOutputFile);
    
}

void program(int fd)
{
    if (isatty(fd)){
        /* RUNNING INTERACTIVE MODE */

        char buffer[BUFFER_SIZE];
        char* string = NULL;
        int bytes = 0;
        ssize_t bytesRead;

        // Initializing shell by printing a welcome message
        write(STDOUT_FILENO, "Initializing shell...\n", 23);
        write(STDOUT_FILENO, "sh> ", 4);

        // Main loop for reading input from stdin (infinite loop until exit command is given)
        while (1) {

            if ((bytesRead = read(STDIN_FILENO, buffer, BUFFER_SIZE)) == -1) {
                perror("Error reading from stdin");
                exit(EXIT_FAILURE);
            }
            
            if ((string = (char*) realloc(string, bytes + bytesRead)) == NULL){
                perror("Error allocating memory");
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

                
                parseCommand(string);  /* Function to parse the job/command */

                bytes = 0;
                free(string);
                string = NULL;
                write(STDOUT_FILENO, "sh> ", 4);

            }
        }
    }else{
        /* RUNNING BATCH MODE */
    }

}

int main(int argc, char* argv[])
{
    if (argc > 2){
        perror("Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    
    if (argc == 2){
        /* BATCH MODE */
        int batch_fd;
        if ((batch_fd = open(argv[1], O_RDONLY)) == -1){
            perror("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        program(batch_fd);

    }else{
        /* INTERACTIVE MODE */
        program(STDIN_FILENO);
    }
}