#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define N 27
#define BUFFER_SIZE 128

int exit_faliure = 0;

typedef struct Trie{
	char* word; //stores the actual word with capitalization
	struct Trie* children[N]; // branches
}Trie;

Trie* createNode()
{
    Trie* node = (Trie*)malloc(sizeof(Trie));

    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    node->word = NULL;

    for(int i = 0; i < N; i++){
        node->children[i] = NULL;
    }

    return node;
}

Trie* insert(Trie* root, char* word)
{
    Trie* node = root;
    
    for(int i = 0; i < strlen(word); i++){
        char c = tolower(word[i]);
        int index = c - 'a';

        if (index < 0 || index > 25) {
            index = 26;
        }

        if(node->children[index] == NULL){
            node->children[index] = createNode();
        }

        node = node->children[index];
        if (index == 26) node->word = &c;
    }

    node->word = word;
    return root;
}

void freeTrie(Trie* root)
{
    if(root == NULL){
        return;
    }

    for(int i = 0; i < N; i++){
        freeTrie(root->children[i]);
    }

    free(root);
}

// returns the word if present in the trie, otherwise returns NULL
char* spellCheck(char* word, Trie* root)
{
    Trie* node = root;

    for(int i = 0; i < strlen(word); i++){
        char c = tolower(word[i]);
        int index = c - 'a';

        if (index < 0 || index > 25) {
            index = 26;
        }

        if(node->children[index] == NULL){
            return NULL;
        }

        node = node->children[index];
    }

    return node->word;
}

char** generate_capitalization_variations(const char* word) 
{

    char** variations = (char**)malloc(3 * sizeof(char*));

    if (variations == NULL) {
        perror("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    // Allocate memory for each variation
    for (int i = 0; i < 3; i++) {
        variations[i] = (char*)malloc((strlen(word) + 1) * sizeof(char));
        if (variations[i] == NULL) {
            perror("Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    
    // Copy the word and generate variations
    strcpy(variations[0], word);  // Add the word as is
    
    variations[1][0] = toupper(word[0]);
    strcpy(variations[1] + 1, word + 1);  // Add the word with initial capitalization

    // Add the word with all letters capitalized if it doesn't contain any lowercase characters
    strcpy(variations[2], word);
    for (int i = 0; i < strlen(word); i++) {
        variations[2][i] = toupper(word[i]);
    }
    return variations;
}

void processText(char* txt, Trie* root, char* fileName)
{
    int line = 1;
    int col = 1;
    char delim[]=" \t\r\n\v\f\0";
    char leadPunc[] = "({[\'\"";
    char* word = NULL;
    int wordLength = 0;
    char* correctSpelling;

    for (int i = 0; i < strlen(txt)+1; i++) {
        if (strchr(delim,txt[i]) == NULL) {
            wordLength++;

        }else{
            if (wordLength == 0) {
                col += wordLength+1;
                if (txt[i] == '\n') {
                    line++;
                    col = 1;
                }
                continue;
            }

            word = (char*) malloc(wordLength+1);
            if (word == NULL) {
                perror("Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            memcpy(word, txt + i - wordLength, wordLength);
            word[wordLength] = '\0';


            //removing leading punctuation
            int wordStart = 0;
            while (strchr(leadPunc,word[wordStart]) != NULL){
                wordStart++;
            }

            int wordEnd = strlen(word)-1;
            while(!isalpha(word[wordEnd])){
                wordEnd--;
            }

            if (wordStart > wordEnd) {
                col += wordLength+1;
                if (txt[i] == '\n') {
                    line++;
                    col = 1;
                }
                free(word);
                wordLength = 0;
                continue;
            }

            char* token = (char*) malloc(wordEnd - wordStart + 2);
            if (token == NULL) {
                perror("Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }

            memcpy(token, word + wordStart, wordEnd - wordStart + 1);
            token[wordEnd - wordStart + 1] = '\0';
            free(word);


            // handling hyphenated words
            if (strchr(token, '-') != NULL){
                
                // creating a copy of word to split
                char* tokenedWord = (char*) malloc(strlen(token) + 1);
                if (tokenedWord == NULL) {
                    perror("Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }

                memcpy(tokenedWord, token, strlen(token));
                tokenedWord[wordLength] = '\0';

                char* tkn = strtok(tokenedWord, "-");
                int incorrect = 0;
                while (tkn != NULL) {
                    correctSpelling = spellCheck(tkn, root);
                    if (correctSpelling != NULL){
                        char** variations = generate_capitalization_variations(correctSpelling);
                        int comp_correct = 0;
                        for (int i = 0; i < 3; i++) {
                            if (strcmp(tkn, variations[i]) == 0) {
                                comp_correct = 1;
                                break;
                            }
                        }
                        if (comp_correct == 0){
                            incorrect = 1;
                        }
                        for (int i = 0; i < 3; i++) {
                            free(variations[i]);
                        }
                        free(variations);
                    }else{
                        printf("%s (%d,%d): %s\n", fileName, line, col, token);
                        exit_faliure = 1;
                        break;
                    }
                    tkn = strtok(NULL, "-");
                }

                if (incorrect == 1){
                    printf("%s (%d,%d): %s\n", fileName, line, col, token);
                    exit_faliure = 1;
                }

                col += wordLength+1;
                if (txt[i] == '\n') {
                    line++;
                    col = 1;
                }
                
                free(token);
                free(tokenedWord);
                wordLength = 0;
                continue;
            }
            

            correctSpelling = spellCheck(token, root);
            if (correctSpelling != NULL){
                char** variations = generate_capitalization_variations(correctSpelling);
                int correct = 0;

                for (int i = 0; i < 3; i++) {
                    if (strcmp(token, variations[i]) == 0) {
                        correct = 1;
                        break;
                    }   
                }
                if (correct == 0) {
                    printf("%s (%d,%d): %s\n", fileName, line, col, token);
                    exit_faliure = 1;
                }
                for (int i = 0; i < 3; i++) {
                    free(variations[i]);
                }
                free(variations);
            }else{
                printf("%s (%d,%d): %s\n", fileName, line, col, token);
                exit_faliure = 1;
            }

            col += wordLength+1;
            if (txt[i] == '\n') {
                line++;
                col = 1;
            }
            free(token);
            wordLength = 0;
        }
    }
}

void traverseDirectory (char* path, Trie* root) 
{
    DIR* dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL){
        
        // creating a path string
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char* filePath = (char*) malloc(strlen(path) + 1 + strlen(entry->d_name) + 1);
        if (filePath == NULL) {
            perror("Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        // copying the path and file name to filePath
        memcpy(filePath, path, strlen(path));
        filePath[strlen(path)] = '/';
        memcpy(filePath + strlen(path) + 1, entry->d_name, strlen(entry->d_name) + 1);
        filePath[strlen(path) + 1 + strlen(entry->d_name)] = '\0';


        if (entry->d_type == DT_REG){
            // logic to read files
            if (entry->d_name[0] != '.' && strcmp((entry->d_name + strlen(entry->d_name)) - 4, ".txt") == 0) {
                // open the file
                int txt_fd = open(filePath, O_RDONLY);
                if (txt_fd == -1) {
                    perror("Error opening text file");
                    exit(EXIT_FAILURE);
                }

                // reading the file
                char buf[BUFFER_SIZE];
                size_t bytesRead;
                size_t totalBytes = 0;
                char* txt = (char*)malloc(1);
                if (txt == NULL) {
                    perror("Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }

                while ((bytesRead = read(txt_fd, buf, sizeof(buf))) > 0) {
                    txt = realloc(txt, totalBytes + bytesRead);
                    if (txt == NULL) {
                        perror("Memory allocation failed\n");
                        exit(EXIT_FAILURE);
                    }

                    memcpy(txt + totalBytes, buf, bytesRead);
                    totalBytes += bytesRead;
                }
                txt = realloc(txt, totalBytes + 1);
                if (txt == NULL) {
                    perror("Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }

                txt[totalBytes] = '\0';
                close(txt_fd);

                // text processing
                processText(txt, root, filePath);
                free(txt);
            }

        } else if (entry->d_type == DT_DIR){
            traverseDirectory(filePath, root);
        }
        free(filePath);
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Check if there are any arguments
    if (argc > 2) {
        int dict_fd = open(argv[1], O_RDONLY);
        if (dict_fd == -1) {
            perror("Error opening dictionary file");
            exit(EXIT_FAILURE);
        }
        char buf[BUFFER_SIZE];
        size_t bytesRead;
        char* words = (char*)malloc(1);
        if (words == NULL) {
            perror("Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        size_t totalBytes = 0;

        while ((bytesRead = read(dict_fd, buf, sizeof(buf))) > 0) {
            words = realloc(words, totalBytes + bytesRead);

            if (words == NULL) {
                perror("Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }

            memcpy(words + totalBytes, buf, bytesRead);
            totalBytes += bytesRead;
        }
        close(dict_fd);
        Trie* root = createNode('\0');
        char* token = strtok(words, "\n");
        while (token != NULL) {
            root = insert(root, token);
            token = strtok(NULL, "\n");
        }


        for (int i = 2; i < argc; i++) {

            struct stat fileStat;
            stat(argv[i], &fileStat);

            // checking if the file is a regular text file
            if (S_ISREG(fileStat.st_mode)) {

                int txt_fd = open(argv[i], O_RDONLY); // open the file
                if (txt_fd == -1) {
                    perror("Error opening text file");
                    return 1;
                }
                
                totalBytes = 0;
                char* txt = (char*) malloc(1);

                // reading the file
                while ((bytesRead = read(txt_fd, buf, sizeof(buf))) > 0) {
                    txt = realloc(txt, totalBytes + bytesRead);
                    if (txt == NULL) {
                        perror("Memory allocation failed\n");
                        exit(EXIT_FAILURE);
                    }

                    memcpy(txt + totalBytes, buf, bytesRead);
                    totalBytes += bytesRead;
                }

                txt = realloc(txt, totalBytes + 1);

                if (txt == NULL) {
                    perror("Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }

                txt[totalBytes] = '\0';
                close(txt_fd);

                // text processing
                processText(txt, root, argv[i]);

                free(txt);

            } else if (S_ISDIR(fileStat.st_mode)) {
                // traverse directory
                traverseDirectory(argv[i], root);
            }
        }

        freeTrie(root);
        free(words);


    } else if (argc > 1) {
        printf("Only one argument found. Enter dict path as first argument and txt file path as subsequent arguments.\n");
    } else {
        printf("No arguments found. Enter dict path as first argument and txt file path as subsequent arguments.\n");
    }

    if (exit_faliure == 1) {
        exit(EXIT_FAILURE);
    }else{
        exit(EXIT_SUCCESS);
    }
}