#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int init_data_files(void) {
    struct stat st = {0};

    // Create data directory if it doesn't exist
    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0755) != 0) {
            perror("Failed to create data directory");
            return 0;
        }
    }

    // Create users file if it doesn't exist
    FILE *f = fopen(USERS_FILE, "a");
    if (!f) {
        perror("Failed to create users file");
        return 0;
    }
    fclose(f);

    // Create blockchain file if it doesn't exist
    f = fopen(BLOCKCHAIN_FILE, "a");
    if (!f) {
        perror("Failed to create blockchain file");
        return 0;
    }
    fclose(f);

    return 1;
}

int load_users(User users[], int *count) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) {
        *count = 0;
        return 1;  // File doesn't exist yet, not an error
    }

    *count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && *count < MAX_USERS) {
        // Remove newline
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
    if (!f) {
        perror("Failed to open users file");
        return 0;
    }
    fprintf(f, "%s:%s\n", user->username, user->password_hash);
    fclose(f);
    return 1;
}

int load_blockchain(Block blocks[], int *count) {
    FILE *f = fopen(BLOCKCHAIN_FILE, "r");
    if (!f) {
        *count = 0;
        return 1;  // File doesn't exist yet, not an error
    }

    *count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), f) && *count < MAX_BLOCKS) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        // Parse: index|timestamp|task|username|completed|prev_hash|hash
        Block *b = &blocks[*count];
        char *token;
        char *rest = line;

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        b->index = atoi(token);

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        b->timestamp = (time_t)atol(token);

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        strncpy(b->task, token, MAX_TASK_LEN - 1);
        b->task[MAX_TASK_LEN - 1] = '\0';

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        strncpy(b->username, token, MAX_USERNAME_LEN - 1);
        b->username[MAX_USERNAME_LEN - 1] = '\0';

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        b->completed = atoi(token);

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        strncpy(b->prev_hash, token, HASH_LEN - 1);
        b->prev_hash[HASH_LEN - 1] = '\0';

        token = strtok_r(rest, "|", &rest);
        if (!token) continue;
        strncpy(b->hash, token, HASH_LEN - 1);
        b->hash[HASH_LEN - 1] = '\0';

        (*count)++;
    }
    fclose(f);
    return 1;
}

int save_block(const Block *block) {
    FILE *f = fopen(BLOCKCHAIN_FILE, "a");
    if (!f) {
        perror("Failed to open blockchain file");
        return 0;
    }
    fprintf(f, "%d|%ld|%s|%s|%d|%s|%s\n",
            block->index,
            (long)block->timestamp,
            block->task,
            block->username,
            block->completed,
            block->prev_hash,
            block->hash);
    fclose(f);
    return 1;
}
