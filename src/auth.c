#include "auth.h"
#include "storage.h"
#include "sha256.h"
#include "blockchain.h"
#include "transaction.h"
#include "mining.h"
#include <stdio.h>
#include <string.h>

static char current_user[MAX_USERNAME_LEN] = "";

int register_user(const char *username, const char *password) {
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Username and password cannot be empty.\n");
        return 0;
    }
    if (strlen(username) >= MAX_USERNAME_LEN) {
        printf("Username too long (max %d characters).\n", MAX_USERNAME_LEN - 1);
        return 0;
    }
    if (!validate_text(username)) {
        printf("Username cannot contain |, ;, or , characters.\n");
        return 0;
    }

    User users[MAX_USERS];
    int count;
    load_users(users, &count);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("Username already exists.\n");
            return 0;
        }
    }

    User new_user;
    strncpy(new_user.username, username, MAX_USERNAME_LEN - 1);
    new_user.username[MAX_USERNAME_LEN - 1] = '\0';
    sha256(password, new_user.password_hash);

    if (!save_user(&new_user)) {
        printf("Failed to save user.\n");
        return 0;
    }

    /* Append a faucet coinbase block giving the new user starting coins */
    Block blocks[MAX_BLOCKS];
    int block_count;
    load_blockchain(blocks, &block_count);

    LedgerModel model;
    int difficulty;
    load_config(&model, &difficulty);

    if (block_count > 0) {
        Transaction tx;
        create_coinbase_tx(&tx, username, FAUCET_AMOUNT);

        Block faucet = create_block(block_count, &tx, "system", difficulty,
                                    blocks[block_count - 1].hash);

        printf("Mining faucet block for %s...\n", username);
        mine_block(&faucet);

        if (!save_block(&faucet)) {
            printf("Warning: Failed to create faucet block.\n");
        } else {
            printf("Faucet: %.2f coins credited to %s.\n", FAUCET_AMOUNT / 100.0, username);
        }
    }

    printf("Registration successful!\n");
    return 1;
}

int login(const char *username, const char *password) {
    if (is_logged_in()) {
        printf("Already logged in as %s. Please logout first.\n", current_user);
        return 0;
    }

    User users[MAX_USERS];
    int count;
    load_users(users, &count);

    char password_hash[HASH_LEN];
    sha256(password, password_hash);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (strcmp(users[i].password_hash, password_hash) == 0) {
                strncpy(current_user, username, MAX_USERNAME_LEN - 1);
                current_user[MAX_USERNAME_LEN - 1] = '\0';
                printf("Login successful! Welcome, %s.\n", current_user);
                return 1;
            } else {
                printf("Invalid password.\n");
                return 0;
            }
        }
    }

    printf("User not found.\n");
    return 0;
}

void logout(void) {
    if (is_logged_in()) {
        printf("Goodbye, %s!\n", current_user);
        current_user[0] = '\0';
    }
}

int is_logged_in(void) {
    return current_user[0] != '\0';
}

const char* get_current_user(void) {
    return current_user;
}
