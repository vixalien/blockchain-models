#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int validate_text(const char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '|' || text[i] == ';' || text[i] == ',') {
            return 0;
        }
    }
    return 1;
}

int init_data_files(void) {
    struct stat st = {0};

    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0755) != 0) {
            perror("Failed to create data directory");
            return 0;
        }
    }

    FILE *f = fopen(USERS_FILE, "a");
    if (!f) { perror("Failed to create users file"); return 0; }
    fclose(f);

    f = fopen(BLOCKCHAIN_FILE, "a");
    if (!f) { perror("Failed to create blockchain file"); return 0; }
    fclose(f);

    return 1;
}

int load_users(User users[], int *count) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) { *count = 0; return 1; }

    *count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && *count < MAX_USERS) {
        line[strcspn(line, "\n")] = 0;
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strncpy(users[*count].username, line, MAX_USERNAME_LEN - 1);
            users[*count].username[MAX_USERNAME_LEN - 1] = '\0';
            strncpy(users[*count].password_hash, colon + 1, HASH_LEN - 1);
            users[*count].password_hash[HASH_LEN - 1] = '\0';
            (*count)++;
        }
    }
    fclose(f);
    return 1;
}

int save_user(const User *user) {
    FILE *f = fopen(USERS_FILE, "a");
    if (!f) { perror("Failed to open users file"); return 0; }
    fprintf(f, "%s:%s\n", user->username, user->password_hash);
    fclose(f);
    return 1;
}

/*
 * Extended block format (17 fields, pipe-delimited):
 * index|timestamp|tx_type|sender|receiver|amount|input_count|INPUTS|output_count|OUTPUTS|task|completed|miner|nonce|difficulty|prev_hash|hash
 *
 * INPUTS:  txhash,idx;txhash,idx;...  (empty if input_count==0)
 * OUTPUTS: recipient,amount;recipient,amount;...  (empty if output_count==0)
 */

int save_block(const Block *block) {
    FILE *f = fopen(BLOCKCHAIN_FILE, "a");
    if (!f) { perror("Failed to open blockchain file"); return 0; }

    /* Basic fields */
    fprintf(f, "%d|%ld|%d|%s|%s|%lld|%d|",
            block->index,
            (long)block->timestamp,
            block->tx.type,
            block->tx.sender,
            block->tx.receiver,
            block->tx.amount,
            block->tx.input_count);

    /* Inputs */
    for (int i = 0; i < block->tx.input_count; i++) {
        if (i > 0) fprintf(f, ";");
        fprintf(f, "%s,%d", block->tx.inputs[i].ref_tx_hash, block->tx.inputs[i].output_index);
    }

    fprintf(f, "|%d|", block->tx.output_count);

    /* Outputs */
    for (int i = 0; i < block->tx.output_count; i++) {
        if (i > 0) fprintf(f, ";");
        fprintf(f, "%s,%lld", block->tx.outputs[i].recipient, block->tx.outputs[i].amount);
    }

    fprintf(f, "|%s|%d|%s|%lu|%d|%s|%s\n",
            block->tx.task,
            block->tx.completed,
            block->miner,
            block->nonce,
            block->difficulty,
            block->prev_hash,
            block->hash);

    fclose(f);
    return 1;
}

/* Helper: parse a single pipe-delimited token from *rest. Returns token or empty string. */
static char *next_token(char **rest) {
    if (*rest == NULL) return "";
    char *start = *rest;
    char *pipe = strchr(start, '|');
    if (pipe) {
        *pipe = '\0';
        *rest = pipe + 1;
    } else {
        *rest = NULL;
    }
    return start;
}

int load_blockchain(Block blocks[], int *count) {
    FILE *f = fopen(BLOCKCHAIN_FILE, "r");
    if (!f) { *count = 0; return 1; }

    *count = 0;
    char line[4096];
    while (fgets(line, sizeof(line), f) && *count < MAX_BLOCKS) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        Block *b = &blocks[*count];
        memset(b, 0, sizeof(Block));
        char *rest = line;

        /* index */
        b->index = atoi(next_token(&rest));
        /* timestamp */
        b->timestamp = (time_t)atol(next_token(&rest));
        /* tx_type */
        b->tx.type = (TxType)atoi(next_token(&rest));
        /* sender */
        strncpy(b->tx.sender, next_token(&rest), MAX_USERNAME_LEN - 1);
        /* receiver */
        strncpy(b->tx.receiver, next_token(&rest), MAX_USERNAME_LEN - 1);
        /* amount */
        b->tx.amount = atoll(next_token(&rest));
        /* input_count */
        b->tx.input_count = atoi(next_token(&rest));

        /* INPUTS field */
        char *inputs_str = next_token(&rest);
        if (b->tx.input_count > 0 && strlen(inputs_str) > 0) {
            char *inp_rest = inputs_str;
            for (int i = 0; i < b->tx.input_count && inp_rest && i < MAX_TX_INPUTS; i++) {
                char *entry = inp_rest;
                char *semi = strchr(inp_rest, ';');
                if (semi) { *semi = '\0'; inp_rest = semi + 1; }
                else { inp_rest = NULL; }

                char *comma = strchr(entry, ',');
                if (comma) {
                    *comma = '\0';
                    strncpy(b->tx.inputs[i].ref_tx_hash, entry, HASH_LEN - 1);
                    b->tx.inputs[i].output_index = atoi(comma + 1);
                }
            }
        }

        /* output_count */
        b->tx.output_count = atoi(next_token(&rest));

        /* OUTPUTS field */
        char *outputs_str = next_token(&rest);
        if (b->tx.output_count > 0 && strlen(outputs_str) > 0) {
            char *out_rest = outputs_str;
            for (int i = 0; i < b->tx.output_count && out_rest && i < MAX_TX_OUTPUTS; i++) {
                char *entry = out_rest;
                char *semi = strchr(out_rest, ';');
                if (semi) { *semi = '\0'; out_rest = semi + 1; }
                else { out_rest = NULL; }

                char *comma = strchr(entry, ',');
                if (comma) {
                    *comma = '\0';
                    strncpy(b->tx.outputs[i].recipient, entry, MAX_USERNAME_LEN - 1);
                    b->tx.outputs[i].amount = atoll(comma + 1);
                }
            }
        }

        /* task */
        strncpy(b->tx.task, next_token(&rest), MAX_TASK_LEN - 1);
        /* completed */
        b->tx.completed = atoi(next_token(&rest));
        /* miner */
        strncpy(b->miner, next_token(&rest), MAX_USERNAME_LEN - 1);
        /* nonce */
        b->nonce = strtoul(next_token(&rest), NULL, 10);
        /* difficulty */
        b->difficulty = atoi(next_token(&rest));
        /* prev_hash */
        strncpy(b->prev_hash, next_token(&rest), HASH_LEN - 1);
        /* hash */
        strncpy(b->hash, next_token(&rest), HASH_LEN - 1);

        /* Recompute tx_hash (not stored separately; derived from tx content) */
        calculate_tx_hash(&b->tx);

        (*count)++;
    }
    fclose(f);
    return 1;
}

int load_config(LedgerModel *model, int *difficulty) {
    /* Defaults */
    *model = LEDGER_UTXO;
    *difficulty = DEFAULT_DIFFICULTY;

    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) return 1;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;

        if (strcmp(key, "ledger_model") == 0) {
            int v = atoi(val);
            if (v == 0 || v == 1) *model = (LedgerModel)v;
        } else if (strcmp(key, "difficulty") == 0) {
            int v = atoi(val);
            if (v >= 1 && v <= 6) *difficulty = v;
        }
    }
    fclose(f);
    return 1;
}

int save_config(LedgerModel model, int difficulty) {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) { perror("Failed to open config file"); return 0; }
    fprintf(f, "ledger_model=%d\n", model);
    fprintf(f, "difficulty=%d\n", difficulty);
    fclose(f);
    return 1;
}
