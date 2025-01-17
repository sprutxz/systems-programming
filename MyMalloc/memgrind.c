#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

#include "mymalloc.h"

int main() {
    printf("new run\n");
    srandom(time(NULL));

    for(int a = 0; a < 1 ; a++){

        printf("PERFORMANCE TEST 1\n");

        clock_t start = clock(); // Start the timer

        for (int j = 0; j < 120; j++) {
            void* p = malloc(1);
            free(p);
        }

        clock_t end = clock(); // Stop the timer
        printf("Time taken(%d): %f seconds\n", a, ((double)(end - start)) / CLOCKS_PER_SEC);

        printf("PERFORMANCE TEST 2\n");

        clock_t start2 = clock(); // Start the timer
        char *obj[120];
        for (int i = 0; i < 120; i++) {
        	obj[i] = malloc(1);
        }
        for (int i = 0; i < 120; i++) {
            free(obj[i]);
        }

        clock_t end2 = clock(); // Stop the timer
        printf("Time taken(%d): %f seconds\n", a, ((double)(end2 - start2)) / CLOCKS_PER_SEC);

        printf("PERFORMANCE TEST 3\n");
        clock_t start3 = clock(); // Start the timer
        int random_number, i, j;
        char* new[120];
        i = 0;
        j = 0;
        while(i < 120) {
            random_number = random() % 2;

            if (random_number == 0) {
                new[i] = malloc(1);
                i++;   
            } else {
                if(i>j){
                    free(new[j]);
                    j++;
                }
            }
        }
        for(int k = j; k < i; k++){
            free(new[k]);
        }

        clock_t end3 = clock(); // Stop the timer
        printf("Time taken(%d): %f seconds\n", a, ((double)(end3 - start3)) / CLOCKS_PER_SEC);


        printf("PERFORMANCE TEST 4\n");
        clock_t start4 = clock(); // Start the timer

        char *obj2[120];
        for(int i = 0; i < 120; i++){
            obj2[i] = malloc(1);
        }
        for(int i = 0; i < 120; i++){
            if (i % 2 == 0) {
                free(obj2[i]);
            }
        }
        for(int i = 0; i < 120; i++){
            if (i % 2 == 1) {
                free(obj2[i]);
            }
        }

        clock_t end4 = clock(); // Stop the timer
        printf("Time taken(%d): %f seconds\n", a, ((double)(end4 - start4)) / CLOCKS_PER_SEC);

        printf("PERFORMANCE TEST 5\n");
        clock_t start5 = clock(); // Start the timer

        i = 0;
        char* obj3[75];

        while(i < 75){
            int rand = random() % 50;
            void* p = malloc(rand);
            if (p == NULL){
                free(obj3[i-1]);
                i--;
            }else{
                obj3[i] = p;
                i++;
            }
        }
        //freeing all the pointers
        for(int i = 0; i < 75; i++){
            free(obj3[i]);
        }

        clock_t end5 = clock(); // Stop the timer
        printf("Time taken(%d): %f seconds\n", a, ((double)(end5 - start5)) / CLOCKS_PER_SEC);
    }



    return 0;
}