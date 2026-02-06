#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "blockchain.h"

#define MAX_ACCOUNTS 200
#define BLOCK_REWARD (50 * COIN)
#define FAUCET_AMOUNT (100 * COIN)

/* ---- UTXO model ---- */

typedef struct {
    char tx_hash[HASH_LEN];
    int output_index;
    char owner[MAX_USERNAME_LEN];
    long long amount;
} UTXO;

#define MAX_UTXOS 2000

typedef struct {
    UTXO utxos[MAX_UTXOS];
    int count;
} UTXOSet;

/* Build UTXO set by scanning all blocks */
void build_utxo_set(Block blocks[], int block_count, UTXOSet *set);

/* Get total balance for a user from UTXO set */
long long get_utxo_balance(const UTXOSet *set, const char *username);

/* Validate a transfer transaction against the UTXO set */
int validate_utxo_tx(const Transaction *tx, const UTXOSet *set);

/* Create a UTXO-based transfer transaction (selects UTXOs, builds inputs/outputs with change) */
int create_utxo_transfer(Transaction *tx, const char *sender, const char *receiver,
                         long long amount, const UTXOSet *set);

/* ---- Account-Balance model ---- */

typedef struct {
    char username[MAX_USERNAME_LEN];
    long long balance;
} Account;

typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int count;
} AccountState;

/* Rebuild account state from entire chain */
void rebuild_account_state(Block blocks[], int block_count, AccountState *state);

/* Update account state with a single new block */
void update_account_state(const Block *block, AccountState *state);

/* Get balance for a user in account model */
long long get_account_balance(const AccountState *state, const char *username);

/* Validate a transfer against account balances */
int validate_account_tx(const Transaction *tx, const AccountState *state);

/* ---- Transaction creation helpers ---- */

/* Create a coinbase transaction (mining reward or faucet) */
void create_coinbase_tx(Transaction *tx, const char *receiver, long long amount);

/* Create a task transaction */
void create_task_tx(Transaction *tx, const char *username, const char *task, int completed);

/* Create a simple transfer transaction for account-balance model (no UTXO inputs/outputs) */
void create_account_transfer_tx(Transaction *tx, const char *sender, const char *receiver,
                                long long amount);

#endif
