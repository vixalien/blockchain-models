#include "todo.h"
#include "blockchain.h"
#include "transaction.h"
#include "mining.h"
#include "storage.h"
#include "auth.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char task[MAX_TASK_LEN];
    int completed;
    int block_index;
} TaskStatus;

int add_task(const char *task) {
    if (!is_logged_in()) {
        printf("You must be logged in to add tasks.\n");
        return 0;
    }

    if (strlen(task) == 0) {
        printf("Task cannot be empty.\n");
        return 0;
    }

    if (!validate_text(task)) {
        printf("Task cannot contain |, ;, or , characters.\n");
        return 0;
    }

    Block blocks[MAX_BLOCKS];
    int count;
    load_blockchain(blocks, &count);

    LedgerModel model;
    int difficulty;
    load_config(&model, &difficulty);

    if (count == 0) {
        Block genesis = create_genesis();
        if (!save_block(&genesis)) {
            printf("Failed to create genesis block.\n");
            return 0;
        }
        blocks[0] = genesis;
        count = 1;
    }

    /* Create TX_TASK transaction */
    Transaction tx;
    create_task_tx(&tx, get_current_user(), task, 0);

    /* Create block and mine it */
    Block new_block = create_block(count, &tx, get_current_user(), difficulty, blocks[count - 1].hash);

    printf("Mining task block (difficulty %d)...\n", difficulty);
    int attempts = mine_block(&new_block);
    printf("Block mined! Attempts: %d\n", attempts);

    if (!save_block(&new_block)) {
        printf("Failed to save task.\n");
        return 0;
    }

    printf("Task added successfully!\n");
    return 1;
}

void view_tasks(void) {
    if (!is_logged_in()) {
        printf("You must be logged in to view tasks.\n");
        return;
    }

    Block blocks[MAX_BLOCKS];
    int count;
    load_blockchain(blocks, &count);

    TaskStatus tasks[MAX_BLOCKS];
    int task_count = 0;
    const char *username = get_current_user();

    for (int i = 0; i < count; i++) {
        if (blocks[i].tx.type != TX_TASK) continue;
        if (strcmp(blocks[i].tx.receiver, username) != 0) continue;
        if (strcmp(blocks[i].tx.task, "Genesis Block") == 0) continue;

        int found = -1;
        for (int j = 0; j < task_count; j++) {
            if (strcmp(tasks[j].task, blocks[i].tx.task) == 0) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            tasks[found].completed = blocks[i].tx.completed;
            tasks[found].block_index = i;
        } else {
            strncpy(tasks[task_count].task, blocks[i].tx.task, MAX_TASK_LEN);
            tasks[task_count].completed = blocks[i].tx.completed;
            tasks[task_count].block_index = i;
            task_count++;
        }
    }

    if (task_count == 0) {
        printf("No tasks found.\n");
        return;
    }

    printf("\n=== Your Tasks ===\n");
    for (int i = 0; i < task_count; i++) {
        printf("%d. [%s] %s\n",
               i + 1,
               tasks[i].completed ? "X" : " ",
               tasks[i].task);
    }
    printf("==================\n\n");
}

int complete_task(int task_num) {
    if (!is_logged_in()) {
        printf("You must be logged in to complete tasks.\n");
        return 0;
    }

    if (task_num < 1) {
        printf("Invalid task number.\n");
        return 0;
    }

    Block blocks[MAX_BLOCKS];
    int count;
    load_blockchain(blocks, &count);

    LedgerModel model;
    int difficulty;
    load_config(&model, &difficulty);

    TaskStatus tasks[MAX_BLOCKS];
    int task_count = 0;
    const char *username = get_current_user();

    for (int i = 0; i < count; i++) {
        if (blocks[i].tx.type != TX_TASK) continue;
        if (strcmp(blocks[i].tx.receiver, username) != 0) continue;
        if (strcmp(blocks[i].tx.task, "Genesis Block") == 0) continue;

        int found = -1;
        for (int j = 0; j < task_count; j++) {
            if (strcmp(tasks[j].task, blocks[i].tx.task) == 0) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            tasks[found].completed = blocks[i].tx.completed;
            tasks[found].block_index = i;
        } else {
            strncpy(tasks[task_count].task, blocks[i].tx.task, MAX_TASK_LEN);
            tasks[task_count].completed = blocks[i].tx.completed;
            tasks[task_count].block_index = i;
            task_count++;
        }
    }

    if (task_num > task_count) {
        printf("Task number %d not found. You have %d tasks.\n", task_num, task_count);
        return 0;
    }

    int idx = task_num - 1;
    if (tasks[idx].completed) {
        printf("Task is already completed.\n");
        return 0;
    }

    Transaction tx;
    create_task_tx(&tx, username, tasks[idx].task, 1);

    Block complete_block = create_block(count, &tx, username, difficulty, blocks[count - 1].hash);

    printf("Mining completion block (difficulty %d)...\n", difficulty);
    mine_block(&complete_block);

    if (!save_block(&complete_block)) {
        printf("Failed to mark task as complete.\n");
        return 0;
    }

    printf("Task '%s' marked as complete!\n", tasks[idx].task);
    return 1;
}

int verify_blockchain(void) {
    Block blocks[MAX_BLOCKS];
    int count;
    load_blockchain(blocks, &count);

    if (count == 0) {
        printf("Blockchain is empty.\n");
        return 1;
    }

    printf("Verifying blockchain with %d blocks...\n", count);
    if (verify_chain(blocks, count)) {
        printf("Blockchain is valid!\n");
        return 1;
    } else {
        printf("Blockchain has been tampered with!\n");
        return 0;
    }
}
