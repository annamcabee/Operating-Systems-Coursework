/* CS 2200 - Project 4 - Spring 2016
 * Name - Anna McAbee
 * GTID - 902944955
 */

#include "cachesim.h"
#include <math.h>


//Cache action
#define READ  		'r'
#define WRITE 		'w'
#define SET_DIRTY  	's'
#define SEE  		'c'

typedef struct {
    uint64_t tag;
    int valid;
    int dirty;
    int num_uses;
} ENTRY_;
typedef struct {
    ENTRY_ * entry;
} CACHEARRAY_;
/**
struct QUEUE {
	unsigned int size;	
	unsigned int maxSize;
	ENTRY *head;
	ENTRY *tail;
	HASH *hash;
};

struct HASH {
	int capacity;
	NODE* array;
};
**/
// global data
uint64_t cache_size;
uint64_t block_size;

uint64_t num_lines;
uint64_t set_size;

uint64_t b;
uint64_t c;

uint64_t block_bit_size;
uint64_t index_bit_size;


uint64_t set_assoc;
int write_miss_num;
int read_miss_num;

CACHEARRAY_* cache;



/**
 * Sub-routine for initializing your cache with the parameters.
 * You may initialize any global variables here.
 *
 * @param C The total size of your cache is 2^C bytes
 * @param S The set associativity is 2^S
 * @param B The size of your block is 2^B bytes
 */
void cache_init(uint64_t C, uint64_t S, uint64_t B) {
	read_miss_num = 0;
	write_miss_num = 0;
    set_assoc = S;
    b = B;
    c = C;
    // fill in
    block_bit_size = B;
    index_bit_size = (C - B - S);
    block_size=pow(2, B);
    cache_size=pow(2, C);	
	set_size = pow(2,S);
	num_lines = cache_size/(block_size*set_size);
	// allocate memory
	int x;
	int y;
	cache = (CACHEARRAY_ *)malloc(num_lines*(sizeof(CACHEARRAY_)));
	for (x = 0; x < num_lines; x++) {
        cache[x].entry = (ENTRY_*)malloc(set_size*sizeof(ENTRY_));
		for (y = 0; y < set_size; y++) {
			cache[x].entry[y].tag = 0;
			cache[x].entry[y].num_uses=0;
			cache[x].entry[y].dirty = 0;
			cache[x].entry[y].valid = 0;
		}
	}
}
/**
 * helper function to calculate tag
 **/
 
uint64_t calculate_tag(uint64_t address) {
	return (address >> (c-set_assoc));
}
/**
 * helper function to calculate index
 **/
uint64_t calculate_index(uint64_t address, uint64_t tag) {
	uint64_t mask = tag << (c - set_assoc - b);
	return ((uint64_t) (address >> b)) & (~mask);
}
 /**
  * uint64_t accesses;
    uint64_t reads;
    uint64_t read_misses;
    uint64_t writes;
    uint64_t write_misses;
    uint64_t misses;
    uint64_t write_backs;   
    uint64_t access_time;
    uint64_t miss_penalty;
    double   miss_rate;
    double   avg_access_time;
  */
 
/**
 * Helper function for reading from a cache
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void read(uint64_t address, struct cache_stats_t *stats, uint64_t tag, uint64_t index_value) {
	// case 1: hit :)
    int i = 0;
    for(i = 0; i < set_size; i++) {
        if (cache[index_value].entry[i].tag == tag && cache[index_value].entry[i].valid == 1) {
            cache[index_value].entry[i].num_uses++;
            stats->reads++;
            return;
        } 
    }
    stats->read_misses++;
    stats->reads++;
    stats->misses++;
    // case 2: check for space in cache
    i = 0;
    for (i = 0; i < set_size; i++) {
        if(cache[index_value].entry[i].valid == 0) {
			 cache[index_value].entry[i].num_uses = 0;
			cache[index_value].entry[i].dirty = 0;
            cache[index_value].entry[i].tag = tag;
            cache[index_value].entry[i].valid = 1;
            return;
        }
    } 
    // case 3 : least frequently used algorithm
    int x = 0;
    uint64_t least_frequent = 0;
    
    for (x =0; x < set_size; x++) {
		if (cache[index_value].entry[x].num_uses < cache[index_value].entry[least_frequent].num_uses) {
			least_frequent = x;
		}
	}
	if (cache[index_value].entry[least_frequent].dirty == 1) stats->write_backs++;
	// bc we are reading, dirty bit set to 0
    cache[index_value].entry[least_frequent].dirty = 0;
	// update other info
    cache[index_value].entry[least_frequent].num_uses = 0;
    cache[index_value].entry[least_frequent].tag = tag;
	
	return;
}

/**
 * Helper function for writing to a cache
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void write(uint64_t address, struct cache_stats_t *stats, uint64_t tag, uint64_t index_value) {
    // case 1 : hit :)
    int i = 0;
    for(i = 0; i < set_size; i++) {
        if (cache[index_value].entry[i].tag == tag && cache[index_value].entry[i].valid == 1) {
            cache[index_value].entry[i].num_uses++;
            cache[index_value].entry[i].dirty=1;
            stats->writes++;
            return;
        }
    }
    stats->write_misses++;
    stats->writes++;
    stats->misses++;
    
    // case 2: check for space in the cache
    int y = 0;
    for (y=0; y < set_size; y++) {
        if(cache[index_value].entry[y].valid == 0) {;
            cache[index_value].entry[y].valid = 1;
            cache[index_value].entry[y].num_uses = 0;
            cache[index_value].entry[y].dirty = 1;
            cache[index_value].entry[y].tag = tag;
            return;
        }
    }
    // case 3 : least frequently used algorithm
    int x = 0;
    uint64_t least_frequent = 0;
    
    for (x =0; x < set_size; x++) {
		if (cache[index_value].entry[x].num_uses < cache[index_value].entry[least_frequent].num_uses) {
			least_frequent = x;
		}
	}
	if (cache[index_value].entry[least_frequent].dirty == 1) stats->write_backs++;
	// bc we are writing, set dirty bit to 1
    cache[index_value].entry[least_frequent].dirty = 1;
    // update entry info
    cache[index_value].entry[least_frequent].num_uses = 0;
    cache[index_value].entry[least_frequent].tag = tag;
	
	return;
}

/**
 * Subroutine that simulates one cache event at a time.
 * @param rw The type of access, READ or WRITE
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void cache_access (char rw, uint64_t address, struct cache_stats_t *stats) {
	stats->accesses++;
	uint64_t tag = calculate_tag(address);
	uint64_t index = calculate_index(address, tag);
	if (rw == READ) {
		read(address, stats, tag, index);
	} else if (rw == WRITE) {
		write(address, stats, tag, index);
	} else {
		printf("Invalid input");
	}
}

/**
 * Subroutine for cleaning up memory operations and doing any calculations
 * Make sure to free malloced memory here.
 *
 */
void cache_cleanup (struct cache_stats_t *stats) {
	// block clean up
	
    int i = 0;
    for (i = 0; i < num_lines; i++) {
		free(cache[i].entry);
	}
	free(cache);
	//printf("hits: %c", hit);
	// calculate stats
	//stats->read_misses=read_miss_num;
	//stats->write_misses=write_miss_num;
    stats->miss_rate = (double) stats->misses/(double) stats->accesses;
    double miss_time =(double)(stats->miss_rate * stats->miss_penalty);
    stats->avg_access_time = (double)stats->access_time + miss_time;

}
