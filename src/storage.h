#ifndef STORAGE_H
#define STORAGE_H

#include "auth.h"
#include "blockchain.h"

#define MAX_BLOCKS 1000
#define DATA_DIR "data"
#define USERS_FILE "data/users.txt"
#define BLOCKCHAIN_FILE "data/blockchain.txt"
#define CONFIG_FILE "data/config.txt"
#define DEFAULT_DIFFICULTY 3

/* Initialize data directory and files */
int init_data_files(void);

/* User persistence */
int load_users(User users[], int *count);
int save_user(const User *user);

/* Blockchain persistence (extended 17-field format) */
int load_blockchain(Block blocks[], int *count);
int save_block(const Block *block);

/* Config persistence (key=value format) */
int load_config(LedgerModel *model, int *difficulty);
int save_config(LedgerModel model, int difficulty);

/* Validate that text contains no forbidden delimiters (|;,) */
int validate_text(const char *text);

#endif
