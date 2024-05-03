#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


// Compile with -DREALMALLOC to use the real malloc() instead of mymalloc()
#ifndef REALMALLOC
#include "mymalloc.h"
#endif

#define MEMSIZE 4096
#define HEADERSIZE 24
#define OBJECTS 64
#define OBJSIZE (MEMSIZE / OBJECTS - HEADERSIZE)

int main(int argc, char **argv)
{   

	char *obj[OBJECTS];
	int i, j, errors = 0;
	
	// fill memory with objects
	for (i = 0; i < OBJECTS; i++) {
		obj[i] = malloc(OBJSIZE);
	}
	// fill each object with distinct bytes
	for (i = 0; i < OBJECTS; i++) {
		memset(obj[i], i, OBJSIZE);
	}
	// check that all objects contain the correct bytes
	for (i = 0; i < OBJECTS; i++) {
		for (j = 0; j < OBJSIZE; j++) {
			if (obj[i][j] != i) {
				errors++;
				printf("Object %d byte %d incorrect: %d\n", i, j, obj[i][j]);
			}
		}
	}
	
	printf("%d incorrect bytes\n", errors);

	// free all objects
	for (i = 0; i < OBJECTS; i++) {
		free(obj[i]);
	}

	printf("Testing if malloc and free function properly\n");
	/* 
	* We first try to request a pointer the size of the whole memory array
	* We then free this array and request another pointer the size of the whole
	* memory array. If free works right, we should get a pointer and not NULL
	*/
	void* p1 = malloc(MEMSIZE-HEADERSIZE);
	free(p1);
	void* p2 = malloc(MEMSIZE-HEADERSIZE);
	if (p2 == NULL) {
		printf("Malloc and free failed\n");
	} else {
		printf("Malloc and free worked\n");
	}

	free(p2);

	printf("Testing if free returns the correct error statements\n");

	//should give a pointer not at start of chunk error
	void* p3 = malloc(sizeof(int));
    free(p3+1);
    free(p3);

    //should give a double free error
    int* p4 = malloc(sizeof(int)*100);
    int* p5 = p4;
    free(p4);
    free(p5);

	//should give a pointer not from malloc error
    double v1;
    free(&v1);



	printf("Testing if coalesce works\n");
	/*
	* We first allocate two pointers that consume the entire memory array
	* Then we free both of these pointers. Next we request another pointer that's the
	* size of the whle array. If the coalesce function is working properly, we should be allocated
	* a pointer and not be return NULL
	*/

	// If coalesce works, z should return a pointer to the start of the memory and not NULL
	void* p6 = malloc((MEMSIZE/2)-HEADERSIZE);
	void* p7 = malloc((MEMSIZE/2)-HEADERSIZE);
	free(p6);
	free(p7);
	void* p8 = malloc(MEMSIZE-HEADERSIZE);
	free(p8);
	if (p6 == NULL) {
		printf("Coalesce failed\n");
	} else {
		printf("Coalesce worked\n");
	}



	return EXIT_SUCCESS;
}
