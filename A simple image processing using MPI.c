#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

int* read_pgm(const char* filename, int* width, int* height, char* header, int header_size){
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

 
}