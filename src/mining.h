#ifndef MINING_H
#define MINING_H

#include "blockchain.h"
#include "transaction.h"

#define MAX_MINERS 20
#define POOL_DEFAULT_FEE_PERCENT 2
#define CLOUD_RENTAL_COST (10 * COIN)
#define CLOUD_MAINTENANCE_FEE (5 * COIN)

typedef struct {
    char username[MAX_USERNAME_LEN];
    int hashrate_share;
} PoolMiner;

typedef struct {
    PoolMiner miners[MAX_MINERS];
    int miner_count;
    int total_shares;
    int pool_fee_percent;
} MiningPool;

typedef struct {
    char renter[MAX_USERNAME_LEN];
    long long rental_cost;
    long long maintenance_fee;
    long long accumulated_reward;
    int active;
} CloudContract;

/* PoW: increment nonce until hash meets difficulty. Returns attempt count. */
int mine_block(Block *block);

/* Solo mine: create coinbase tx, build block, run PoW, append to chain */
int solo_mine(Block blocks[], int *block_count, const char *miner, int difficulty);

/* Initialize an empty mining pool */
void init_pool(MiningPool *pool);

/* Add a miner to the pool */
int add_pool_miner(MiningPool *pool, const char *username, int hashrate_share);

/* Pool mine: run PoW, append single coinbase block, print reward distribution */
int pool_mine(Block blocks[], int *block_count, MiningPool *pool, int difficulty);

/* Initialize a cloud contract */
void init_cloud_contract(CloudContract *contract, const char *renter);

/* Cloud mine: run PoW, append block, print fee deductions */
int cloud_mine(Block blocks[], int *block_count, CloudContract *contract, int difficulty);

/* Print mining statistics for a user */
void print_mining_stats(Block blocks[], int block_count, const char *username);

#endif
