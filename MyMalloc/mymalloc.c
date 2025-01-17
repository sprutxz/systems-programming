#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "mymalloc.h"

#define MEMLENGTH 4096 // size of memory
static double memory[MEMLENGTH];

static const size_t align = 8; // 8 byte alignment

typedef struct chunk_header {
    size_t size; //size of the block, it will include the size of the header
    int free; // 0 if not free, 1 if free
    struct chunk_header *next; //pointer to the next block
}header;

//initialization function
void initialize() {
    header* head = (header*)memory;
    head->size = MEMLENGTH;
    head->free = 1;
    head->next = NULL;
}

//function to align the memory
size_t alignSize(size_t size) {
    return (size + align - 1) & ~(align - 1);
}

//function to split the chunk
header* splitChunk(header* chunk, size_t size) {
    header* new = (header*)((char*)(chunk) + size);
    new->size = chunk->size - size;
    new->free = 1;
    new->next = chunk->next;
    return new;
}

//function to coalesce the adjacent free chunks
void coalesce(){
    header* ptr = (header*)memory;

    while(ptr != NULL){

        //checking is current pointer is free
        if(ptr->free == 1){

            //if the next chunk is free, we coalesce the chunks
            if(ptr->next && ptr->next->free == 1){
                ptr->size += ptr->next->size;
                ptr->next = ptr->next->next;
            }
            
            if(ptr->next && ptr->next->free == 1){
                continue;
            }
        }
        ptr = ptr->next; //move to the next chunk
    }
}

void *mymalloc(size_t size, char *file, int line) {
    if (size == 0) {
        return NULL;
    }

    //calculating the size of the chunk with alignment and adding the size of the header
    size = alignSize(size);
    size += alignSize(sizeof(header));

    header* ptr = (header*)memory; 

    //if the memory is not initialized, we initialize it
    if (ptr->size == 0){
        initialize();
    }

    while (ptr){

         //checking if the chunk is free and is bigger/equal than the required size
        if (ptr->free && ptr->size >= size){

            //if the chunk size is equal to the required size, we return the pointer to the chunk
            if(ptr->size == size){ 
                ptr->free = 0;
                return (void*) ((char*)ptr + alignSize(sizeof(header)));
            }

            //if the chunk size is bigger than the required size, we split the chunk and return the pointer to the chunk
            header* newChunk = splitChunk(ptr, size);

            ptr->size = size;
            ptr->free = 0;
            ptr->next = newChunk;

            //returning the pointer to the chunk
            return (void*) ((char*)ptr + alignSize(sizeof(header)));
        }
        ptr = ptr->next;
    }

    return NULL; //no free chunk found
}

void myfree(void *ptr, char *file, int line) {
    //checking if the pointer is NULL
    if (ptr == NULL) {
        return;
    }

    header* chunk = (header*)((char*)ptr - alignSize(sizeof(header)));
    if (ptr >= (void*)memory && ptr < (void*)(memory + MEMLENGTH)){
        

        if(chunk->size < alignSize(sizeof(header))){
            printf("Error while calling free:\npointer not at the start of chunk (%s:%d)\n", file, line);
            return;
        }
        if (chunk->free == 1){
            printf("Error while calling free:\ntried to free a pointer that is already free (%s:%d)\n", file, line);
            return;
        }
        chunk->free = 1;
        
    }else{
        printf("Error while calling free:\npointer not obtained from malloc (%s:%d)\n", file, line);
        return;
    }

    coalesce();
}