#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glob.h>
#include <dirent.h>

#define BUFFER_SIZE 128

typedef struct glob_expansion {
    char** globv;
    int globc;
} GlobExp;

void freeGlobExp (GlobExp* pattern_matches)
{
    for (int i = 0; i < pattern_matches->globc; i++){
        free(pattern_matches->globv[i]);
    }
    free(pattern_matches->globv);
    free(pattern_matches);
}

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

int matchPattern (char* pattern, char* file)
{
    char* ptr1 = pattern;
    char* ptr2 = file;

    if (*ptr1 == '*' && *ptr2 == '.'){
        return 0;
    }

    while (*ptr1 != '\0' && *ptr2 != '\0'){
        if (*ptr1 == '*'){
            ptr1++;
            if (*ptr1 == '\0'){
                return 1;
            }

            while (*ptr2 != *ptr1){
                ptr2++;
                if (*ptr2 == '\0'){
                    return 0;
                }
            }

        }else if (*ptr1 == *ptr2){
            ptr1++;
            ptr2++;
        }else{
            return 0;
        }
    }
    if (*ptr1 == *ptr2){
        return 1;
    } else {
        return 0;
    }
}

GlobExp* ExpandPattern (char* file_pattern){
    GlobExp* pattern_matches = (GlobExp*) malloc(sizeof(GlobExp));
    pattern_matches->globv = NULL;
    pattern_matches->globc = 0;

    char* ptr = NULL;
    char* path = NULL;
    ptr = strrchr(file_pattern, '/');
    DIR* dir;
    if (ptr != NULL){
        path = (char*) malloc(ptr - file_pattern + 1);
        memcpy(path, file_pattern, ptr - file_pattern);
        path[ptr - file_pattern] = '\0';

        if ((dir = opendir(path)) == NULL){
            perror("Error opening directory\n");
            exit(EXIT_FAILURE);
        }
        
    } else {

        if ((dir = opendir(".")) == NULL){
            perror("Error opening directory\n");
            exit(EXIT_FAILURE);
        }
    }

    char* pattern = (ptr == NULL ? file_pattern : ptr + 1);

    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        if (entry->d_type != DT_REG){
            continue;
        }

        if (matchPattern(pattern, entry->d_name)){
            pattern_matches->globc++;

            pattern_matches->globv = (char**) realloc(pattern_matches->globv, pattern_matches->globc * sizeof(char*));
            if (pattern_matches->globv == NULL){
                perror("Error allocating memory\n");
                exit(EXIT_FAILURE);
            }

            if (path == NULL){

                pattern_matches->globv[count] = strdup(entry->d_name);
                count++;

            } else {

                size_t path_len = strlen(path);
                size_t file_len = strlen(entry->d_name);
                size_t total_path_length = path_len + file_len + 2;

                pattern_matches->globv[count] = (char*) malloc(total_path_length * sizeof(char));
                memcpy(pattern_matches->globv[count], path, path_len);
                pattern_matches->globv[count][path_len] = '/';
                memcpy(pattern_matches->globv[count] + path_len + 1, entry->d_name, file_len);
                pattern_matches->globv[count][total_path_length-1] = '\0';

                count++;
            }
        }
    }

    if (pattern_matches->globc == 0){
        pattern_matches->globc = 1;
        pattern_matches->globv = (char**) malloc(sizeof(char*));
        pattern_matches->globv[0] = strdup(file_pattern);
    }

    closedir(dir);
    return pattern_matches;
}

char* dupetoken(char* token)
{
    unsigned token_len = strlen(token);
    if (token[0] == '\"'){
        token++;
        token_len--;
    }
    if (token[token_len - 1] == '\"'){
        token_len--;
    }

    char* dup = (char*) malloc(strlen(token) + 1);
    if (dup == NULL){
        perror("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    
    memcpy(dup, token, token_len);
    dup[token_len] = '\0';

    return dup;
}

char** tokenize (char* command, int numArgs)
{
    char** tokens = (char**) malloc((numArgs + 1) * sizeof(char*));
    int i = 0;

    char* token = strtok(command, " ");
    while (token != NULL){
        if (strchr(token, '*')){

            GlobExp* pattern_matches = ExpandPattern(token);

            numArgs += pattern_matches->globc - 1;

            tokens = (char**) realloc(tokens, (numArgs + 1) * sizeof(char*));
            if (tokens == NULL){
                perror("Error allocating memory\n");
                exit(EXIT_FAILURE);
            }

            for (int j = 0; j < pattern_matches->globc; j++){
                tokens[i] = strdup(pattern_matches->globv[j]);
                i++;
            }

            freeGlobExp(pattern_matches);
            token = strtok(NULL, " ");
            continue;

        }

        tokens[i] = dupetoken(token);
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

void exit_program(char* string) {
    /* We print every argument provided seprated by spaces */

    char delim[] = " \t\r\n\v\f\0|<>";

    int word_len = 0;
    for (unsigned i = 0; i < strlen(string)+1; i++){
        if (strchr(delim,string[i]) == NULL){
            word_len++;
        } else if (string[i] == '<'){
            printf("< ");
            word_len = 0;
        } else if (string[i] == '>'){
            printf("> ");
            word_len = 0;
        } else if (string[i] == '|'){
            printf("| ");
            word_len = 0;
        } else {
            if (word_len > 0){
                write(STDOUT_FILENO, string + i - word_len, word_len);
                write(STDOUT_FILENO, " ", 1);
                word_len = 0;
            }
        }
    }
    write(STDOUT_FILENO, "Exiting shell...\n", 18);
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

    if (strcmp (cmd, "exit") == 0){
        return 4;
    }

    return 0;

}

void BuiltInFunctions (char** args, int numArgs, int index)
{
    switch(index){
        case 1:
            if (numArgs != 2){
                perror("Invalid number of arguments\n");
                break;
            }
            if (chdir(args[1]) != 0){
                perror("Error changing directory");
            }
            break;

        case 2:
            if (numArgs != 1){
                perror("Invalid number of arguments\n");
                break;
            }

            char* cwd = getcwd(NULL, 0);
            if (cwd == NULL){
                perror("Error getting current working directory\n");
            }

            write(STDOUT_FILENO, cwd, strlen(cwd));
            write(STDOUT_FILENO, "\n", 1);
            free(cwd);

            break;

        case 3:
            if (numArgs != 2){
                perror("Invalid number of arguments\n");
                break;
            }
            char* pathToExec = BareNameSearch(args[1]);

            if (pathToExec != NULL){
            write(STDOUT_FILENO, pathToExec, strlen(pathToExec));
            write(STDOUT_FILENO, "\n", 1);
            }

            free(pathToExec);
            break;
        
        case 4:
            for (int i = 1; i < numArgs; i++){
                write(STDOUT_FILENO, args[i], strlen(args[i]));
                write(STDOUT_FILENO, " ", 1);
            }

            exit(EXIT_SUCCESS);
            break;
        
        default:
            break;
    }
}

void openAndRedirect(char* file, int input_or_output)
{
    int fd;

    if (input_or_output == 1){

        fd = open(file, O_RDONLY); // open file for reading

        if (fd == -1){
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }

        /* duping and closing the old descriptor */
        dup2(fd, STDIN_FILENO);
        close(fd);

    }else{

        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0640); // open file for writing

        if (fd == -1){
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }

        /* duping and closing the old descriptor */
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


    /* When the pipe_flag is set to 0 (no pipes in command) we execute the command
       normally with just 1 child process, else we create two child processes and
       execute both commands seprately with first child output being pipes to the 
       other child */

    int exit_status; // variable to store the exit status of the last child process

    if (pipe_flag == 0){

        char** tokens = tokenize(command, numArgs[0]); // we tokenize the command string to get the arguments

        /* We check if the command is a builtin command.
           If builtin then execute in parent else create
           a child process and execute the command */
        int i;
        if ((i = isBuiltin(tokens[0])) != 0){

            int save_stdout;
            int save_stdin;
            if (outputFile != NULL){
                save_stdout = dup(STDOUT_FILENO);
                openAndRedirect(outputFile, 0);
            }
            if (inputFile != NULL){
                save_stdin = dup(STDIN_FILENO);
                openAndRedirect(inputFile, 1);
            }

            BuiltInFunctions(tokens, numArgs[0], i);

            if (outputFile != NULL){
                dup2(save_stdout, STDOUT_FILENO);
                close(save_stdout);
            }
            if (inputFile != NULL){
                dup2(save_stdin, STDIN_FILENO);
                close(save_stdin);
            }

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

                free(pathToExec); // freeing the path to the executable file after use

            }
        }

        wait(&exit_status); // wait for the child process to finish and store the exit status
        freeTokens(tokens, numArgs[0]); // free the tokens

    } else { // executed if pipe_flag is set to 1

        char** tokens = tokenize(command, numArgs[0]); // argument list for first job
        char** pipetokens = tokenize(command + flag + 1, numArgs[1]); // argument list for second job

        int pipefd[2]; // the pipe

        if (pipe(pipefd) == -1){
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        int i;
        /* FIRST CHILD PROCESS */
        if ((i = isBuiltin(tokens[0])) != 0){
            int save_stdout = dup(STDOUT_FILENO);

            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
            close(pipefd[1]); // Close the write end of the pipe

            BuiltInFunctions(tokens, numArgs[0], i);

            dup2(save_stdout, STDOUT_FILENO); // restoring standard output
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
            
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the read end of pipe
            close(pipefd[0]);

            BuiltInFunctions(pipetokens, numArgs[1], i);

            dup2(save_stdin, STDIN_FILENO); // restore standard input
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

                /* Redirecting pipes before files allows file redirection to take precedence */
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

                // if (strstr(string, "exit") != 0){
                //     // Make exit print all argument it recieves then exit
                //     exit_program(string + 4);
                //     free(string);
                //     break;
                // }

                parseCommand(string);  /* Function to parse the job/command */

                bytes = 0;
                free(string);
                string = NULL;
                write(STDOUT_FILENO, "sh> ", 4);

            }
        }
    }else{
        /* RUNNING BATCH MODE */
        
        int buffer_size = BUFFER_SIZE;
        char* buffer = (char*) malloc(buffer_size * sizeof(char));
        
        int bytes_read;
        int curr_index;
        int start_index;

        /* Code for reading lines similar to the one provided by the professor */
        while ((bytes_read =  read(fd, buffer + curr_index, buffer_size - curr_index)) > 0) {
            start_index = 0;
            int buffer_end = curr_index + bytes_read;

            while (curr_index < buffer_end){

                if (buffer[curr_index] == '\n') {

                    buffer[curr_index] = '\0';
                    parseCommand(buffer + start_index);
                    start_index = curr_index + 1;

                }

                curr_index++;
            }

            if (start_index == curr_index) {

                curr_index = 0;

            }else if (start_index > 0) {

                int segment_length = curr_index - start_index;
                memmove(buffer, buffer + start_index, segment_length);
                curr_index = segment_length;

            } else if (buffer_end == buffer_size) {

                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);

            }
        }

        if (curr_index > 0) {
            if (curr_index == buffer_size) {
                buffer = realloc(buffer, buffer_size + 1);
            }
            buffer[curr_index] = '\0';
            parseCommand(buffer + start_index);
        }

        free(buffer);
        
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
        int batch_file;
        if ((batch_file = open(argv[1], O_RDONLY)) == -1){
            perror("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        program(batch_file);

    }else{
        /* INTERACTIVE MODE */
        program(STDIN_FILENO);
    }
}