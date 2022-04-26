/*
* Tanner Turba
* April 21, 2022
* Project 2 -
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "project2.h"

#define LINELEN 1024

int main() {
    // open "parameters.txt" and save to params
    FILE * params = fopen("parameters.txt", "r");
    if(params == NULL) {
        perror("Error opening file.\n");
        return 1;
    }

    // open "accesses.txt" and save to accesses
    FILE * accesses = fopen("accesses.txt", "r");
    if(accesses == NULL) {
        perror("Error opening file.\n");
        return 1;
    }

    // get data from parameter file
    interpret_parameters(params);
    
    // get data from the accesses file
    interpret_input(accesses);

    // printf("\nassociativity = directMapped: %d\n", associativity == directMapped);
    // printf("offset_bits: %d\n", offset_bits);
    // printf("index_bits: %d\n", index_bits);
    // printf("allocation = writeAllocate: %d\n", allocation == writeAllocate);
    // printf("writePolicy = writeBack: %d\n", writePolicy = writeBack);

    // begin simulation
    simulate();

    // print stats
    print_out_data();

    // free all data
    cleanup();
}

void simulate() {
    // create the cache using index_bits to find number of sets
    int num_of_sets = find_num_of_sets(index_bits);
    Set * cache = (Set *)malloc(sizeof(Set) * num_of_sets); 

    // decide the way_size, based on associativity type
    int way_size;
    switch(associativity) {
        case directMapped:
            way_size = 1;
            break;

        case twoWay:
            way_size = 2;
            break;

        case fourWay:
            way_size = 4;
            break;
    }

    // initialize valid and dirty bits
    int i;
    for(i = 0; i < num_of_sets; i++) {
        Set current_set = cache[i];
        int x;
        for(x = 0; x < way_size; x++) {
            current_set.valid[x] = 0;
            current_set.dirty[x] = 0;
        }
    }


    for(i = 0; i < file_length; i++) {
        // for each instruction in the file, get the components
        unsigned int index = find_index(addresses[i]);
        unsigned int offset = find_offset(addresses[i]);
        unsigned int tag = find_tag(addresses[i]);
        char access_type = types[i];

        // index into a set from the cache and loop through its contents
        Set selected_set = cache[index];
        selected_set.last_accessed++;
        int j, hit = 0;
        for(j = 0; j < way_size && hit == 0; j++) {
            unsigned int valid_bit = selected_set.valid[j];
            int dirty_bit = selected_set.dirty[j];
            unsigned int * tag_bit = &selected_set.tag[j];

            // determing hit or miss
            hit = (valid_bit == 1) && (* tag_bit == tag);

            // allocation actions
            switch (allocation) {
                case writeAllocate:
                    if(hit == 1 && access_type == 'r') {
                        // read hit: read block, do nothing
                        printf("writeAllocate - read hit\n");
                        rhits++;
                    } 
                    else if(hit == 0 && access_type == 'r') {
                        // read miss: allocate block/update tag & valid bit
                        printf("writeAllocate - read miss\n");
                        selected_set.tag[j] = tag;
                        selected_set.valid[j] = 1;
                        rmisses++;
                    }
                    else if(hit == 1 && access_type == 'w') {
                        // write hit: do nothing
                        printf("writeAllocate - write hit\n");
                        whits++;
                    }
                    else if(hit == 0 && access_type == 'w') {
                        // write miss: allocate block/update tag & valid bit
                        printf("writeAllocate - write miss\n");
                        selected_set.tag[j] = tag;
                        selected_set.valid[j] = 1;
                        wmisses++;
                    }
                    break;
                
                case writeNoAllocate:
                    if(hit == 1 && access_type == 'r') {
                        // read hit: read block, do nothing
                        printf("writeNoAllocate - read hit\n");
                        rhits++;
                    } 
                    else if(hit == 0 && access_type == 'r') {
                        // read miss: update tag & valid bit
                        printf("writeNoAllocate - read miss\n");
                        selected_set.valid[j] = 1;
                        rmisses++;
                    }
                    else if(hit == 1 && access_type == 'w') {
                        // write hit: write back(do nothing)
                        printf("writeNoAllocate - write hit\n");
                        whits++;
                    }
                    else if(hit == 0 && access_type == 'w') {
                        // write miss: do nothing
                        printf("writeNoAllocate - write miss\n");
                        wmisses++;
                    }
                    break;
            }

            // write policy
            switch (writePolicy) {
                case writeThrough:
                    if(hit == 1 && access_type == 'r') {
                        // read hit: tag in cache, do nothing
                        printf("writeThrough - read hit\n");
                        rhits++;
                    } 
                    else if(hit == 0 && access_type == 'r') {
                        // read miss: do nothing
                        printf("writeThrough - read miss\n");
                        rmisses++;
                    }
                    else if(hit == 1 && access_type == 'w') {
                        // write hit: write through, do nothing
                        printf("writeThrough - write hit\n");
                        whits++;
                        wt++;
                    }
                    else if(hit == 0 && access_type == 'w') {
                        // write miss: depends on allocation policy
                        printf("writeThrough - write miss\n");
                        switch (allocation) {
                            case writeAllocate:
                                //write the tag & valid = 1
                                selected_set.tag[j] = tag;
                                selected_set.valid[j] = 1;
                                break;
                            
                            case writeNoAllocate:
                                //do nothing
                                break;
                        }
                        wt++;
                        wmisses++;
                    }
                    break;
                
                case writeBack:
                    if(dirty_bit == 1 && hit == 1 && access_type == 'r') {
                        // read hit: update dirty bit
                        printf("writeBack - read hit\n");
                        selected_set.dirty[j] = 0;
                        rhits++;
                    } 
                    else if(dirty_bit == 0 && hit == 0 && access_type == 'r') {
                        // read miss: do nothing
                        printf("writeBack - read miss\n");
                        selected_set.dirty[j] = 0;
                        rmisses++;
                    }
                    else if(dirty_bit == 1 && hit == 1 && access_type == 'w') {
                        // write hit: write currebt block to next level, update dirty bit
                        printf("writeBack - write hit\n");
                        selected_set.tag[j] = tag;
                        selected_set.dirty[j] = 1;
                        wb++;
                        whits++;
                    }
                    else if(dirty_bit == 0 && hit == 0 && access_type == 'w') {
                        // write miss: depends on allocation policy
                        printf("writeBack - write miss\n");
                        switch (allocation) {
                            case writeAllocate:
                                //write the tag & valid = 1
                                selected_set.tag[j] = tag;
                                selected_set.valid[j] = 1;
                                break;
                            
                            case writeNoAllocate:
                                //do nothing
                                wt++;
                                break;
                        }
                        wmisses++;
                    }
                    break; 
            }
        }
    }
}

void print_out_data() {
    FILE *fout = fopen("statistics.txt", "w");
    fprintf(fout, "rhits: %d\n", rhits);
    fprintf(fout, "whits: %d\n", whits);
    fprintf(fout, "rmisses: %d\n", rmisses);
    fprintf(fout, "wmisses: %d\n", wmisses);
    fprintf(fout, "hrate: %f\n", (double)(rhits + whits) / (double)(rhits + whits + rmisses + wmisses));
    fprintf(fout, "wb: %d\n", wb);
    fprintf(fout, "wt: %d\n", wt);
}

int find_num_of_sets(int index_bits) {
    int i, num_of_sets = 1;
    for(i = 0; i < index_bits; i++) {
        num_of_sets = num_of_sets * 2;
    }
    return num_of_sets;
}

unsigned int find_offset(unsigned int addr) {
    addr = addr << (32 - offset_bits);
    return addr >> (32 - offset_bits);
}

unsigned int find_index(unsigned int addr) {
    unsigned int tag_size = 32 - index_bits - offset_bits;
    addr = addr << tag_size;
    return addr >> (32 - index_bits);
}

unsigned int find_tag(unsigned int addr) {
    return addr >> (index_bits + offset_bits);
}

void interpret_input(FILE * infile) {
    //parse through the file to count the length
    char blank[LINELEN];
    while( fscanf(infile, "%s", blank) != EOF) {
        file_length++;
    }
    fseek(infile, 0, SEEK_SET);

    //allocate space for arrays
    addresses = (unsigned int *)malloc(sizeof(unsigned int) * file_length);
    types = (char *)malloc(sizeof(char) * file_length);

    //while not the EOF, read values to each corresponding array
    int i = 0;
    while( feof(infile) == 0 ) {
        fscanf(infile, "%s %x", &types[i], &addresses[i]);
        i += 1;
    }
}

void interpret_parameters(FILE * infile) {
    // read each parameter from text
    char * lineOne = get_data(infile);
    fscanf(infile, "%d", &offset_bits);
    fscanf(infile, "%d", &index_bits);
    get_data(infile);
    char * lineFour = get_data(infile);
    char * lineFive = get_data(infile);

    // printf("\nlineOne: %s\n", lineOne);
    // printf("offset_bits: %d\n", offset_bits);
    // printf("index_bits: %d\n", index_bits);
    // printf("lineFour: %s\n", lineFour);
    // printf("lineFive: %s\n", lineFive);

    // determine assosciativity 
    if(strcmp("1", lineOne)) {
        associativity = directMapped;
    } else if(strcmp("2", lineOne)) {
        associativity = twoWay;
    } else if(strcmp("4", lineOne)) {
        associativity = fourWay;
    }

    // determine allocation policy
    if('a' == (int)lineFour[1]) {
        allocation = writeAllocate;
    } else if('n' == (int)lineFour[1]) {
        allocation = writeNoAllocate;
    }

    //determine write policy
    if('t' == (int)lineFive[1]) {
        writePolicy = writeThrough;
    } else if('b' == (int)lineFive[1]) {
        writePolicy = writeBack;
    }
}

char * get_data(FILE * infile) {
    char buffer[LINELEN]; 
    char * fgets_rtn = fgets(buffer, LINELEN, infile);
    buffer[strcspn(buffer, "\n")] = 0;
    return strdup(fgets_rtn);
}

void cleanup() {
    free(addresses);
    free(types);
}