#include "blockchain.h"
#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void calculate_tx_hash(Transaction *tx) {
    /* Build string: "{type}{sender}{receiver}{amount}{input_count}[inputs...]{output_count}[outputs...]{task}{completed}" */
    char data[4096];
    int pos = 0;

    pos += snprintf(data + pos, sizeof(data) - pos, "%d%s%s%lld%d",
                    tx->type, tx->sender, tx->receiver, tx->amount, tx->input_count);

    for (int i = 0; i < tx->input_count; i++) {
        pos += snprintf(data + pos, sizeof(data) - pos, "%s:%d",
                        tx->inputs[i].ref_tx_hash, tx->inputs[i].output_index);
    }

    pos += snprintf(data + pos, sizeof(data) - pos, "%d", tx->output_count);

    for (int i = 0; i < tx->output_count; i++) {
        pos += snprintf(data + pos, sizeof(data) - pos, "%s:%lld",
                        tx->outputs[i].recipient, tx->outputs[i].amount);
    }

    snprintf(data + pos, sizeof(data) - pos, "%s%d", tx->task, tx->completed);

    sha256(data, tx->tx_hash);
}

/* Compute block hash from: "{index}{timestamp}{tx_hash}{miner}{nonce}{difficulty}{prev_hash}" */
static void block_hash_string(const Block *block, char *data, size_t size) {
    snprintf(data, size, "%d%ld%s%s%lu%d%s",
             block->index,
             (long)block->timestamp,
             block->tx.tx_hash,
             block->miner,
             block->nonce,
             block->difficulty,
             block->prev_hash);
}

void calculate_hash(Block *block) {
    char data[2048];
    block_hash_string(block, data, sizeof(data));
    sha256(data, block->hash);
}

void compute_block_hash(const Block *block, char *out_hash) {
    char data[2048];
    block_hash_string(block, data, sizeof(data));
    sha256(data, out_hash);
}

int hash_meets_difficulty(const char *hash, int difficulty) {
    for (int i = 0; i < difficulty; i++) {
        if (hash[i] != '0') return 0;
    }
    return 1;
}

Block create_genesis(void) {
    Block genesis;
    memset(&genesis, 0, sizeof(Block));

    genesis.index = 0;
    genesis.timestamp = time(NULL);
    genesis.difficulty = 0;
    genesis.nonce = 0;
    strncpy(genesis.miner, "system", MAX_USERNAME_LEN - 1);

    genesis.tx.type = TX_TASK;
    strncpy(genesis.tx.task, "Genesis Block", MAX_TASK_LEN - 1);
    genesis.tx.amount = 0;
    genesis.tx.completed = 0;

    memset(genesis.prev_hash, '0', 64);
    genesis.prev_hash[64] = '\0';

    calculate_tx_hash(&genesis.tx);
    calculate_hash(&genesis);

    return genesis;
}

Block create_block(int index, Transaction *tx, const char *miner,
                   int difficulty, const char *prev_hash) {
    Block block;
    memset(&block, 0, sizeof(Block));

    block.index = index;
    block.timestamp = time(NULL);
    block.difficulty = difficulty;
    block.nonce = 0;

    memcpy(&block.tx, tx, sizeof(Transaction));

    strncpy(block.miner, miner, MAX_USERNAME_LEN - 1);
    block.miner[MAX_USERNAME_LEN - 1] = '\0';

    strncpy(block.prev_hash, prev_hash, HASH_LEN - 1);
    block.prev_hash[HASH_LEN - 1] = '\0';

    calculate_tx_hash(&block.tx);
    calculate_hash(&block);

    return block;
}

int verify_chain(Block blocks[], int count) {
    if (count == 0) return 1;

    for (int i = 0; i < count; i++) {
        /* Recompute tx_hash and verify block hash using compute_block_hash (no mutation) */
        char computed_tx_hash[HASH_LEN];
        Transaction tx_copy;
        memcpy(&tx_copy, &blocks[i].tx, sizeof(Transaction));
        calculate_tx_hash(&tx_copy);
        strncpy(computed_tx_hash, tx_copy.tx_hash, HASH_LEN);

        if (strcmp(computed_tx_hash, blocks[i].tx.tx_hash) != 0) {
            printf("Block %d has invalid transaction hash!\n", i);
            return 0;
        }

        char computed_hash[HASH_LEN];
        compute_block_hash(&blocks[i], computed_hash);

        if (strcmp(computed_hash, blocks[i].hash) != 0) {
            printf("Block %d has invalid hash!\n", i);
            printf("Expected: %s\n", computed_hash);
            printf("Found:    %s\n", blocks[i].hash);
            return 0;
        }

        /* Check PoW for blocks with difficulty > 0 */
        if (blocks[i].difficulty > 0) {
            if (!hash_meets_difficulty(blocks[i].hash, blocks[i].difficulty)) {
                printf("Block %d does not meet difficulty requirement!\n", i);
                return 0;
            }
        }

        /* Verify chain linkage (except genesis) */
        if (i > 0) {
            if (strcmp(blocks[i].prev_hash, blocks[i - 1].hash) != 0) {
                printf("Block %d has invalid previous hash link!\n", i);
                return 0;
            }
        }
    }
    return 1;
}
