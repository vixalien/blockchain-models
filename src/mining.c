#include "mining.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>

int mine_block(Block *block) {
    int attempts = 0;
    block->nonce = 0;
    do {
        block->nonce++;
        calculate_hash(block);
        attempts++;
    } while (!hash_meets_difficulty(block->hash, block->difficulty));
    return attempts;
}

int solo_mine(Block blocks[], int *block_count, const char *miner, int difficulty) {
    if (*block_count >= MAX_BLOCKS) {
        printf("Blockchain is full!\n");
        return 0;
    }

    /* Create coinbase transaction */
    Transaction tx;
    create_coinbase_tx(&tx, miner, BLOCK_REWARD);

    /* Create and mine the block */
    char *prev_hash = blocks[*block_count - 1].hash;
    Block block = create_block(*block_count, &tx, miner, difficulty, prev_hash);

    printf("Mining block %d (difficulty %d)...\n", block.index, difficulty);
    int attempts = mine_block(&block);
    printf("Block mined! Nonce: %lu, Attempts: %d\n", block.nonce, attempts);
    printf("Hash: %s\n", block.hash);
    printf("Reward: %.2f coins\n", BLOCK_REWARD / 100.0);

    /* Save and add to in-memory array */
    if (!save_block(&block)) {
        printf("Failed to save mined block.\n");
        return 0;
    }
    blocks[*block_count] = block;
    (*block_count)++;

    return 1;
}

void init_pool(MiningPool *pool) {
    pool->miner_count = 0;
    pool->total_shares = 0;
    pool->pool_fee_percent = POOL_DEFAULT_FEE_PERCENT;
}

int add_pool_miner(MiningPool *pool, const char *username, int hashrate_share) {
    if (pool->miner_count >= MAX_MINERS) {
        printf("Pool is full!\n");
        return 0;
    }
    if (hashrate_share <= 0) {
        printf("Hashrate share must be positive.\n");
        return 0;
    }

    strncpy(pool->miners[pool->miner_count].username, username, MAX_USERNAME_LEN - 1);
    pool->miners[pool->miner_count].username[MAX_USERNAME_LEN - 1] = '\0';
    pool->miners[pool->miner_count].hashrate_share = hashrate_share;
    pool->total_shares += hashrate_share;
    pool->miner_count++;

    return 1;
}

int pool_mine(Block blocks[], int *block_count, MiningPool *pool, int difficulty) {
    if (*block_count >= MAX_BLOCKS) {
        printf("Blockchain is full!\n");
        return 0;
    }
    if (pool->miner_count == 0) {
        printf("No miners in pool!\n");
        return 0;
    }

    /* On-chain: single coinbase to the first miner (pool operator) */
    Transaction tx;
    create_coinbase_tx(&tx, pool->miners[0].username, BLOCK_REWARD);

    char *prev_hash = blocks[*block_count - 1].hash;
    Block block = create_block(*block_count, &tx, pool->miners[0].username, difficulty, prev_hash);

    printf("Pool mining block %d (difficulty %d)...\n", block.index, difficulty);
    int attempts = mine_block(&block);
    printf("Block mined! Nonce: %lu, Attempts: %d\n", block.nonce, attempts);
    printf("Hash: %s\n", block.hash);

    if (!save_block(&block)) {
        printf("Failed to save mined block.\n");
        return 0;
    }
    blocks[*block_count] = block;
    (*block_count)++;

    /* Off-chain reward distribution display */
    long long pool_fee = (BLOCK_REWARD * pool->pool_fee_percent) / 100;
    long long distributable = BLOCK_REWARD - pool_fee;

    printf("\n=== Pool Reward Distribution ===\n");
    printf("Block reward:  %.2f coins\n", BLOCK_REWARD / 100.0);
    printf("Pool fee (%d%%): %.2f coins\n", pool->pool_fee_percent, pool_fee / 100.0);
    printf("Distributable: %.2f coins\n", distributable / 100.0);
    printf("--------------------------------\n");

    for (int i = 0; i < pool->miner_count; i++) {
        long long share = (distributable * pool->miners[i].hashrate_share) / pool->total_shares;
        printf("  %s (share %d/%d): %.2f coins\n",
               pool->miners[i].username,
               pool->miners[i].hashrate_share,
               pool->total_shares,
               share / 100.0);
    }
    printf("================================\n");

    return 1;
}

void init_cloud_contract(CloudContract *contract, const char *renter) {
    memset(contract, 0, sizeof(CloudContract));
    strncpy(contract->renter, renter, MAX_USERNAME_LEN - 1);
    contract->rental_cost = CLOUD_RENTAL_COST;
    contract->maintenance_fee = CLOUD_MAINTENANCE_FEE;
    contract->accumulated_reward = 0;
    contract->active = 1;
}

int cloud_mine(Block blocks[], int *block_count, CloudContract *contract, int difficulty) {
    if (*block_count >= MAX_BLOCKS) {
        printf("Blockchain is full!\n");
        return 0;
    }
    if (!contract->active) {
        printf("No active cloud contract!\n");
        return 0;
    }

    /* Create coinbase to the renter */
    Transaction tx;
    create_coinbase_tx(&tx, contract->renter, BLOCK_REWARD);

    char *prev_hash = blocks[*block_count - 1].hash;
    Block block = create_block(*block_count, &tx, contract->renter, difficulty, prev_hash);

    printf("Cloud mining block %d (difficulty %d)...\n", block.index, difficulty);
    int attempts = mine_block(&block);
    printf("Block mined! Nonce: %lu, Attempts: %d\n", block.nonce, attempts);
    printf("Hash: %s\n", block.hash);

    if (!save_block(&block)) {
        printf("Failed to save mined block.\n");
        return 0;
    }
    blocks[*block_count] = block;
    (*block_count)++;

    /* Off-chain fee accounting */
    long long net_reward = BLOCK_REWARD - contract->rental_cost - contract->maintenance_fee;
    contract->accumulated_reward += net_reward;

    printf("\n=== Cloud Mining Summary ===\n");
    printf("Gross reward:      %.2f coins\n", BLOCK_REWARD / 100.0);
    printf("Rental cost:      -%.2f coins\n", contract->rental_cost / 100.0);
    printf("Maintenance fee:  -%.2f coins\n", contract->maintenance_fee / 100.0);
    printf("Net reward:        %.2f coins\n", net_reward / 100.0);
    printf("Accumulated total: %.2f coins\n", contract->accumulated_reward / 100.0);
    printf("============================\n");

    return 1;
}

void print_mining_stats(Block blocks[], int block_count, const char *username) {
    int blocks_mined = 0;
    long long total_rewards = 0;

    printf("\n=== Mining Statistics for %s ===\n", username);

    for (int i = 0; i < block_count; i++) {
        if (blocks[i].tx.type == TX_COINBASE &&
            strcmp(blocks[i].miner, username) == 0) {
            blocks_mined++;
            total_rewards += blocks[i].tx.amount;
        }
    }

    printf("Blocks mined: %d\n", blocks_mined);
    printf("Total rewards: %.2f coins\n", total_rewards / 100.0);

    /* Recent mining history (last 5) */
    printf("\nRecent mining history:\n");
    int shown = 0;
    for (int i = block_count - 1; i >= 0 && shown < 5; i--) {
        if (blocks[i].tx.type == TX_COINBASE &&
            strcmp(blocks[i].miner, username) == 0) {
            printf("  Block %d: %.2f coins (nonce: %lu, difficulty: %d)\n",
                   blocks[i].index, blocks[i].tx.amount / 100.0,
                   blocks[i].nonce, blocks[i].difficulty);
            shown++;
        }
    }
    if (shown == 0) printf("  No mining history.\n");
    printf("================================\n");
}
