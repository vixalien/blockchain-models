#include "todo.h"
#include "blockchain.h"
#include "storage.h"
#include "auth.h"
#include <stdio.h>
#include <string.h>

// Helper structure to track unique tasks and their completion status
typedef struct {
    char task[MAX_TASK_LEN];
    int completed;
    int block_index;  // Index in blockchain for reference
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

    Block blocks[MAX_BLOCKS];
    int count;
    load_blockchain(blocks, &count);

    // Get previous hash (from last block or genesis)
    char prev_hash[HASH_LEN];
    if (count > 0) {
        strncpy(prev_hash, blocks[count - 1].hash, HASH_LEN);
    } else {
        // Create genesis block first
        Block genesis = create_genesis();
        if (!save_block(&genesis)) {
            printf("Failed to create genesis block.\n");
            return 0;
        }
        strncpy(prev_hash, genesis.hash, HASH_LEN);
        count = 1;
    }

    Block new_block = create_block(count, task, get_current_user(), prev_hash, 0);
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

    // Collect unique tasks for current user with latest status
    TaskStatus tasks[MAX_BLOCKS];
    int task_count = 0;

    const char *username = get_current_user();

    for (int i = 0; i < count; i++) {
        if (strcmp(blocks[i].username, username) != 0) continue;
        if (strcmp(blocks[i].task, "Genesis Block") == 0) continue;

        // Check if this task already exists in our list
        int found = -1;
        for (int j = 0; j < task_count; j++) {
            if (strcmp(tasks[j].task, blocks[i].task) == 0) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            // Update completion status (later blocks have precedence)
            tasks[found].completed = blocks[i].completed;
            tasks[found].block_index = i;
        } else {
            // Add new task
            strncpy(tasks[task_count].task, blocks[i].task, MAX_TASK_LEN);
            tasks[task_count].completed = blocks[i].completed;
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

    // Build task list (same logic as view_tasks)
    TaskStatus tasks[MAX_BLOCKS];
    int task_count = 0;
    const char *username = get_current_user();

    for (int i = 0; i < count; i++) {
        if (strcmp(blocks[i].username, username) != 0) continue;
        if (strcmp(blocks[i].task, "Genesis Block") == 0) continue;

        int found = -1;
        for (int j = 0; j < task_count; j++) {
            if (strcmp(tasks[j].task, blocks[i].task) == 0) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            tasks[found].completed = blocks[i].completed;
            tasks[found].block_index = i;
        } else {
            strncpy(tasks[task_count].task, blocks[i].task, MAX_TASK_LEN);
            tasks[task_count].completed = blocks[i].completed;
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

    // Create a new block marking the task as complete
    // This preserves blockchain immutability
    char prev_hash[HASH_LEN];
    strncpy(prev_hash, blocks[count - 1].hash, HASH_LEN);

    Block complete_block = create_block(count, tasks[idx].task, username, prev_hash, 1);
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
