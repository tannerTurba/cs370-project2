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

void debug(Set * cache, int way_size) {
    int j;
    for(j = 0; j < way_size; j++) {
        int valid_bit = cache[0].valid[j];
        int dirty_bit = cache[0].dirty[j];
        unsigned int tag_bit = cache[0].tag[j];

        // printf("valid_bit = %d \ndirty_bit = %d \ntag_bit = %x\n\n", valid_bit, dirty_bit, tag_bit);
    }
}

int get_replacement_index(Set * cache, int cache_index, int way_index, int way_size) {
    // pop the replacement index from the front of the queue array
    int replacement_index = cache[cache_index].fifo[0];

    // pull forward each remaining element in queue
    int i;
    for(i = 0; i + 1 < way_size; i++) {
        cache[cache_index].fifo[i] = cache[cache_index].fifo[i + 1];
    }
    // place the current index(most recently accessed) in the last index of the queue
    cache[cache_index].fifo[way_size - 1] = way_index;

    return replacement_index;
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
        int x;
        for(x = 0; x < way_size; x++) {
            cache[i].valid[x] = 0;
            cache[i].dirty[x] = 0;
            cache[i].fifo[x] = x;
        }
    }

    for(i = 0; i < file_length; i++) {
        // for each instruction in the file, get the components
        unsigned int index = find_index(addresses[i]);
        unsigned int offset = find_offset(addresses[i]);
        unsigned int tag = find_tag(addresses[i]);
        char access_type = types[i];

        // index into a set from the cache and loop through its contents
        int j, hit = 0;
        for(j = 0; j < way_size; j++) {
            unsigned int valid_bit = cache[index].valid[j];
            int dirty_bit = cache[index].dirty[j];
            unsigned int tag_bit = cache[index].tag[j];
            // debug(cache, way_size); 

            // determing hit or miss
            hit = ((valid_bit == 1) && (tag_bit == tag));
            // printf("%x(valid_bit) == 1 && %x(tag_bit) == %x(tag)\n", valid_bit, tag_bit, tag);
            // printf("hit = %d\n", hit);

            if(access_type == 'r') {
                if(hit == 1) {
                    // read hit
                    rhits++;
                }
                else if(hit == 0) {
                    // read miss
                    rmisses++;

                    // get the index of the block in the set that will be over written
                    int replace_index = get_replacement_index(cache, index, j, way_size);

                    // determine if that block is being replaced
                    int is_replaced = cache[index].valid[replace_index] == 1;

                    // replace tag in that block and update valid bit
                    cache[index].tag[replace_index] = tag;
                    cache[index].valid[replace_index] = 1;

                    // check replacement 
                    if(is_replaced == 1) {
                        switch(writePolicy) {
                            case writeThrough:
                                // do nothing
                                break;

                            case writeBack:
                                // update dirty bit
                                cache[index].dirty[replace_index] = 0;
                                wb++;
                                break;
                        }
                    }
                    else if(is_replaced == 0) {
                        // do nothing
                    }
                }
            }
            else if(access_type == 'w') {
                if(hit == 1) {
                    // write hit
                    whits++;
                    switch(writePolicy) {
                        case writeThrough:
                            // increment count
                            wt++;
                            break;

                        case writeBack:
                            // do nothing
                            break;
                    }
                }
                else if(hit == 0) {
                    // write miss
                    wmisses++;
                    switch(allocation) {
                        case writeAllocate:
                            printf("");
                            // get the index of the block in the set that will be over written
                            int replace_index = get_replacement_index(cache, index, j, way_size);

                            // determine if that block is being replaced
                            int is_replaced = cache[index].valid[replace_index] == 1;

                            // update tag, update valid bit
                            cache[index].tag[replace_index] = tag;
                            cache[index].valid[replace_index] = 1;
                            
                            if(is_replaced) {
                                switch(writePolicy) {
                                    case writeThrough:
                                        // increment count
                                        wt++;
                                        break;

                                    case writeBack:
                                        // update dirty bit
                                        cache[index].dirty[replace_index] = 1;
                                        wb++;
                                        break;
                                }
                            }
                            break;

                        case writeNoAllocate:
                            // do nothing
                            break;
                    }
                }
            }
            // debug(cache, way_size);
            // printf("____________________________\n");
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
    file_length = file_length / 2;
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