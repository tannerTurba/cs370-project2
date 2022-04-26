#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// methods
void simulate();
void print_out_data();
int find_num_of_sets(int index_bits);
unsigned int find_offset(unsigned int addr);
unsigned int find_index(unsigned int addr);
unsigned int find_tag(unsigned int addr);
void interpret_input(FILE * infile);
void interpret_parameters(FILE * infile);
char * get_data(FILE * infile);
void cleanup();

// enums to interpret data from parameter file
enum associativityType{directMapped, twoWay, fourWay};
enum allocationPolicyType{writeAllocate, writeNoAllocate};
enum writePolicyType{writeThrough, writeBack};

// global variables
enum associativityType associativity;
int offset_bits;
int index_bits;
enum allocationPolicyType allocation;
enum writePolicyType writePolicy;
int file_length;
unsigned int * addresses;
char * types;
int rhits = 0;
int whits = 0;
int rmisses = 0;
int wmisses = 0;
int wb = 0;
int wt = 0;

// cache struct model
struct Set {
    int valid[4]; 
    int dirty[4];
    unsigned int tag[4];
    int last_accessed;
};
typedef struct Set Set;
