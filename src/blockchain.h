#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <time.h>

#define MAX_TASK_LEN 256
#define MAX_USERNAME_LEN 50
#define HASH_LEN 65
#define MAX_TX_INPUTS 10
#define MAX_TX_OUTPUTS 10
#define COIN 100LL

typedef enum { TX_TASK = 0, TX_TRANSFER = 1, TX_COINBASE = 2 } TxType;
typedef enum { LEDGER_UTXO = 0, LEDGER_ACCOUNT = 1 } LedgerModel;

typedef struct {
    char ref_tx_hash[HASH_LEN];
    int output_index;
} TxInput;

typedef struct {
    char recipient[MAX_USERNAME_LEN];
    long long amount;
} TxOutput;

typedef struct {
    TxType type;
    char sender[MAX_USERNAME_LEN];
    char receiver[MAX_USERNAME_LEN];
    long long amount;
    TxInput inputs[MAX_TX_INPUTS];
    int input_count;
    TxOutput outputs[MAX_TX_OUTPUTS];
    int output_count;
    char task[MAX_TASK_LEN];
    int completed;
    char tx_hash[HASH_LEN];
} Transaction;

typedef struct Block {
    int index;
    time_t timestamp;
    Transaction tx;
    char miner[MAX_USERNAME_LEN];
    unsigned long nonce;
    int difficulty;
    char prev_hash[HASH_LEN];
    char hash[HASH_LEN];
} Block;

/* Compute tx_hash from transaction content only */
void calculate_tx_hash(Transaction *tx);

/* Compute block hash and store in block->hash */
void calculate_hash(Block *block);

/* Compute block hash into out_hash without mutating block */
void compute_block_hash(const Block *block, char *out_hash);

/* Check if hash has required leading zeros */
int hash_meets_difficulty(const char *hash, int difficulty);

/* Create the genesis block */
Block create_genesis(void);

/* Create a new block with a transaction */
Block create_block(int index, Transaction *tx, const char *miner,
                   int difficulty, const char *prev_hash);

/* Verify entire blockchain integrity */
int verify_chain(Block blocks[], int count);

#endif
