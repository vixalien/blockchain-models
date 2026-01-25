#ifndef STORAGE_H
#define STORAGE_H

#include "auth.h"
#include "blockchain.h"

#define MAX_BLOCKS 1000
#define DATA_DIR "data"
#define USERS_FILE "data/users.txt"
#define BLOCKCHAIN_FILE "data/blockchain.txt"

// Initialize data directory and files if they don't exist
int init_data_files(void);

// Load users from file (returns count of users loaded)
int load_users(User users[], int *count);

// Save a new user to file (append)
int save_user(const User *user);

// Load blockchain from file (returns count of blocks loaded)
int load_blockchain(Block blocks[], int *count);

// Save a new block to file (append)
int save_block(const Block *block);

#endif
