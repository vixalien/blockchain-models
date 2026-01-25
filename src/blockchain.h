#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <time.h>

#define MAX_TASK_LEN 256
#define MAX_USERNAME_LEN 50
#define HASH_LEN 65

typedef struct Block {
    int index;
    time_t timestamp;
    char task[MAX_TASK_LEN];
    char username[MAX_USERNAME_LEN];
    int completed;
    char prev_hash[HASH_LEN];
    char hash[HASH_LEN];
} Block;

// Calculate hash for a block based on its fields
void calculate_hash(Block *block);

// Create the genesis block (first block in chain)
Block create_genesis(void);

// Create a new block with task data
Block create_block(int index, const char *task, const char *username, const char *prev_hash, int completed);

// Verify entire blockchain integrity
int verify_chain(Block blocks[], int count);

#endif
