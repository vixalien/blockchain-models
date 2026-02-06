#include "transaction.h"
#include <stdio.h>
#include <string.h>

/* ---- UTXO model ---- */

void build_utxo_set(Block blocks[], int block_count, UTXOSet *set) {
    set->count = 0;

    for (int b = 0; b < block_count; b++) {
        Transaction *tx = &blocks[b].tx;

        /* Add outputs for TX_COINBASE and TX_TRANSFER */
        if (tx->type == TX_COINBASE || tx->type == TX_TRANSFER) {
            for (int o = 0; o < tx->output_count; o++) {
                if (set->count >= MAX_UTXOS) break;
                UTXO *u = &set->utxos[set->count];
                strncpy(u->tx_hash, tx->tx_hash, HASH_LEN - 1);
                u->tx_hash[HASH_LEN - 1] = '\0';
                u->output_index = o;
                strncpy(u->owner, tx->outputs[o].recipient, MAX_USERNAME_LEN - 1);
                u->owner[MAX_USERNAME_LEN - 1] = '\0';
                u->amount = tx->outputs[o].amount;
                set->count++;
            }
        }

        /* Remove spent UTXOs for TX_TRANSFER */
        if (tx->type == TX_TRANSFER) {
            for (int i = 0; i < tx->input_count; i++) {
                for (int u = 0; u < set->count; u++) {
                    if (strcmp(set->utxos[u].tx_hash, tx->inputs[i].ref_tx_hash) == 0 &&
                        set->utxos[u].output_index == tx->inputs[i].output_index) {
                        /* Remove by swapping with last */
                        set->utxos[u] = set->utxos[set->count - 1];
                        set->count--;
                        break;
                    }
                }
            }
        }
    }
}

long long get_utxo_balance(const UTXOSet *set, const char *username) {
    long long balance = 0;
    for (int i = 0; i < set->count; i++) {
        if (strcmp(set->utxos[i].owner, username) == 0) {
            balance += set->utxos[i].amount;
        }
    }
    return balance;
}

int validate_utxo_tx(const Transaction *tx, const UTXOSet *set) {
    if (tx->type != TX_TRANSFER) return 1;

    /* Check for duplicate inputs within the transaction */
    for (int i = 0; i < tx->input_count; i++) {
        for (int j = i + 1; j < tx->input_count; j++) {
            if (strcmp(tx->inputs[i].ref_tx_hash, tx->inputs[j].ref_tx_hash) == 0 &&
                tx->inputs[i].output_index == tx->inputs[j].output_index) {
                printf("Error: Duplicate input detected in transaction!\n");
                return 0;
            }
        }
    }

    /* Check each input exists in UTXO set and belongs to sender */
    long long input_total = 0;
    for (int i = 0; i < tx->input_count; i++) {
        int found = 0;
        for (int u = 0; u < set->count; u++) {
            if (strcmp(set->utxos[u].tx_hash, tx->inputs[i].ref_tx_hash) == 0 &&
                set->utxos[u].output_index == tx->inputs[i].output_index) {
                if (strcmp(set->utxos[u].owner, tx->sender) != 0) {
                    printf("Error: Input does not belong to sender!\n");
                    return 0;
                }
                input_total += set->utxos[u].amount;
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Error: Input references non-existent UTXO (double-spend attempt)!\n");
            return 0;
        }
    }

    /* Conservation of value: sum(inputs) == sum(outputs) */
    long long output_total = 0;
    for (int i = 0; i < tx->output_count; i++) {
        output_total += tx->outputs[i].amount;
    }

    if (input_total != output_total) {
        printf("Error: Input total (%lld) != output total (%lld)!\n", input_total, output_total);
        return 0;
    }

    return 1;
}

int create_utxo_transfer(Transaction *tx, const char *sender, const char *receiver,
                         long long amount, const UTXOSet *set) {
    memset(tx, 0, sizeof(Transaction));
    tx->type = TX_TRANSFER;
    strncpy(tx->sender, sender, MAX_USERNAME_LEN - 1);
    strncpy(tx->receiver, receiver, MAX_USERNAME_LEN - 1);
    tx->amount = amount;

    /* Gather sender's UTXOs until we have enough */
    long long gathered = 0;
    tx->input_count = 0;

    for (int u = 0; u < set->count && gathered < amount; u++) {
        if (strcmp(set->utxos[u].owner, sender) == 0) {
            if (tx->input_count >= MAX_TX_INPUTS) {
                printf("Error: Too many inputs needed. Consolidate UTXOs first.\n");
                return 0;
            }
            TxInput *inp = &tx->inputs[tx->input_count];
            strncpy(inp->ref_tx_hash, set->utxos[u].tx_hash, HASH_LEN - 1);
            inp->output_index = set->utxos[u].output_index;
            gathered += set->utxos[u].amount;
            tx->input_count++;
        }
    }

    if (gathered < amount) {
        printf("Error: Insufficient funds! Have %.2f, need %.2f\n",
               gathered / 100.0, amount / 100.0);
        return 0;
    }

    /* Create output to receiver */
    tx->output_count = 0;
    tx->outputs[tx->output_count].amount = amount;
    strncpy(tx->outputs[tx->output_count].recipient, receiver, MAX_USERNAME_LEN - 1);
    tx->output_count++;

    /* Change output back to sender if needed */
    if (gathered > amount) {
        tx->outputs[tx->output_count].amount = gathered - amount;
        strncpy(tx->outputs[tx->output_count].recipient, sender, MAX_USERNAME_LEN - 1);
        tx->output_count++;
    }

    return 1;
}

/* ---- Account-Balance model ---- */

static int find_or_create_account(AccountState *state, const char *username) {
    for (int i = 0; i < state->count; i++) {
        if (strcmp(state->accounts[i].username, username) == 0) return i;
    }
    if (state->count >= MAX_ACCOUNTS) return -1;
    int idx = state->count;
    strncpy(state->accounts[idx].username, username, MAX_USERNAME_LEN - 1);
    state->accounts[idx].username[MAX_USERNAME_LEN - 1] = '\0';
    state->accounts[idx].balance = 0;
    state->count++;
    return idx;
}

void rebuild_account_state(Block blocks[], int block_count, AccountState *state) {
    state->count = 0;

    for (int b = 0; b < block_count; b++) {
        update_account_state(&blocks[b], state);
    }
}

void update_account_state(const Block *block, AccountState *state) {
    const Transaction *tx = &block->tx;

    if (tx->type == TX_COINBASE) {
        int idx = find_or_create_account(state, tx->receiver);
        if (idx >= 0) state->accounts[idx].balance += tx->amount;
    } else if (tx->type == TX_TRANSFER) {
        int sidx = find_or_create_account(state, tx->sender);
        int ridx = find_or_create_account(state, tx->receiver);
        if (sidx >= 0) state->accounts[sidx].balance -= tx->amount;
        if (ridx >= 0) state->accounts[ridx].balance += tx->amount;
    }
    /* TX_TASK has no balance effect */
}

long long get_account_balance(const AccountState *state, const char *username) {
    for (int i = 0; i < state->count; i++) {
        if (strcmp(state->accounts[i].username, username) == 0) {
            return state->accounts[i].balance;
        }
    }
    return 0;
}

int validate_account_tx(const Transaction *tx, const AccountState *state) {
    if (tx->type != TX_TRANSFER) return 1;

    long long balance = get_account_balance(state, tx->sender);
    if (balance < tx->amount) {
        printf("Error: Insufficient funds! Have %.2f, need %.2f\n",
               balance / 100.0, tx->amount / 100.0);
        return 0;
    }
    return 1;
}

/* ---- Transaction creation helpers ---- */

void create_coinbase_tx(Transaction *tx, const char *receiver, long long amount) {
    memset(tx, 0, sizeof(Transaction));
    tx->type = TX_COINBASE;
    strncpy(tx->receiver, receiver, MAX_USERNAME_LEN - 1);
    tx->amount = amount;
    tx->input_count = 0;
    tx->output_count = 1;
    strncpy(tx->outputs[0].recipient, receiver, MAX_USERNAME_LEN - 1);
    tx->outputs[0].amount = amount;
}

void create_task_tx(Transaction *tx, const char *username, const char *task, int completed) {
    memset(tx, 0, sizeof(Transaction));
    tx->type = TX_TASK;
    strncpy(tx->receiver, username, MAX_USERNAME_LEN - 1);
    strncpy(tx->task, task, MAX_TASK_LEN - 1);
    tx->completed = completed;
    tx->amount = 0;
}

void create_account_transfer_tx(Transaction *tx, const char *sender, const char *receiver,
                                long long amount) {
    memset(tx, 0, sizeof(Transaction));
    tx->type = TX_TRANSFER;
    strncpy(tx->sender, sender, MAX_USERNAME_LEN - 1);
    strncpy(tx->receiver, receiver, MAX_USERNAME_LEN - 1);
    tx->amount = amount;
    /* Account model doesn't use UTXO inputs/outputs */
    tx->input_count = 0;
    tx->output_count = 0;
}
