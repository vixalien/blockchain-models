#include "blockchain.h"
#include "crypto.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void calculate_hash(Block *block) {
    char data[1024];
    snprintf(data, sizeof(data), "%d%ld%s%s%d%s",
             block->index,
             (long)block->timestamp,
             block->task,
             block->username,
             block->completed,
             block->prev_hash);
    sha256(data, block->hash);
}

Block create_genesis(void) {
    Block genesis;
    genesis.index = 0;
    genesis.timestamp = time(NULL);
    strncpy(genesis.task, "Genesis Block", MAX_TASK_LEN - 1);
    genesis.task[MAX_TASK_LEN - 1] = '\0';
    strncpy(genesis.username, "system", MAX_USERNAME_LEN - 1);
    genesis.username[MAX_USERNAME_LEN - 1] = '\0';
    genesis.completed = 0;
    memset(genesis.prev_hash, '0', 64);
    genesis.prev_hash[64] = '\0';
    calculate_hash(&genesis);
    return genesis;
}

Block create_block(int index, const char *task, const char *username, const char *prev_hash, int completed) {
    Block block;
    block.index = index;
    block.timestamp = time(NULL);
    strncpy(block.task, task, MAX_TASK_LEN - 1);
    block.task[MAX_TASK_LEN - 1] = '\0';
    strncpy(block.username, username, MAX_USERNAME_LEN - 1);
    block.username[MAX_USERNAME_LEN - 1] = '\0';
    block.completed = completed;
    strncpy(block.prev_hash, prev_hash, HASH_LEN - 1);
    block.prev_hash[HASH_LEN - 1] = '\0';
    calculate_hash(&block);
    return block;
}

int verify_chain(Block blocks[], int count) {
    if (count == 0) return 1;

    for (int i = 0; i < count; i++) {
        // Verify hash integrity
        char computed_hash[HASH_LEN];
        char original_hash[HASH_LEN];
        strncpy(original_hash, blocks[i].hash, HASH_LEN);
        calculate_hash(&blocks[i]);
        strncpy(computed_hash, blocks[i].hash, HASH_LEN);
        strncpy(blocks[i].hash, original_hash, HASH_LEN);

        if (strcmp(computed_hash, original_hash) != 0) {
            printf("Block %d has invalid hash!\n", i);
            printf("Expected: %s\n", computed_hash);
            printf("Found:    %s\n", original_hash);
            return 0;
        }

        // Verify chain linkage (except genesis)
        if (i > 0) {
            if (strcmp(blocks[i].prev_hash, blocks[i-1].hash) != 0) {
                printf("Block %d has invalid previous hash link!\n", i);
                return 0;
            }
        }
    }
    return 1;
}
