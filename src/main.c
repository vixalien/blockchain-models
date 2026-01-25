#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"
#include "todo.h"
#include "storage.h"
#include "blockchain.h"

#define MAX_INPUT 256

// Get string input safely
static void get_input(const char *prompt, char *buffer, int max_len) {
    printf("%s", prompt);
    if (fgets(buffer, max_len, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

// Get integer input with validation
static int get_int_input(const char *prompt) {
    char buffer[32];
    get_input(prompt, buffer, sizeof(buffer));
    return atoi(buffer);
}

// Auth menu (before login)
static void auth_menu(void) {
    printf("\n=== Blockchain Todo App ===\n");
    printf("1. Login\n");
    printf("2. Register\n");
    printf("3. Exit\n");
    printf("===========================\n");
}

// Todo menu (after login)
static void todo_menu(void) {
    printf("\n=== Todo Menu (%s) ===\n", get_current_user());
    printf("1. Add Task\n");
    printf("2. View Tasks\n");
    printf("3. Complete Task\n");
    printf("4. Verify Blockchain\n");
    printf("5. Logout\n");
    printf("==========================\n");
}

// Handle authentication actions
static int handle_auth(void) {
    int choice = get_int_input("Choose option: ");
    char username[MAX_USERNAME_LEN];
    char password[MAX_INPUT];

    switch (choice) {
        case 1:  // Login
            get_input("Username: ", username, MAX_USERNAME_LEN);
            get_input("Password: ", password, MAX_INPUT);
            login(username, password);
            break;

        case 2:  // Register
            get_input("Username: ", username, MAX_USERNAME_LEN);
            get_input("Password: ", password, MAX_INPUT);
            register_user(username, password);
            break;

        case 3:  // Exit
            printf("Goodbye!\n");
            return 0;

        default:
            printf("Invalid option. Please try again.\n");
    }
    return 1;
}

// Handle todo actions
static int handle_todo(void) {
    int choice = get_int_input("Choose option: ");
    char task[MAX_TASK_LEN];
    int task_num;

    switch (choice) {
        case 1:  // Add Task
            get_input("Enter task: ", task, MAX_TASK_LEN);
            add_task(task);
            break;

        case 2:  // View Tasks
            view_tasks();
            break;

        case 3:  // Complete Task
            view_tasks();
            task_num = get_int_input("Enter task number to complete: ");
            complete_task(task_num);
            break;

        case 4:  // Verify Blockchain
            verify_blockchain();
            break;

        case 5:  // Logout
            logout();
            break;

        default:
            printf("Invalid option. Please try again.\n");
    }
    return 1;
}

int main(void) {
    // Initialize data files
    if (!init_data_files()) {
        fprintf(stderr, "Failed to initialize data files.\n");
        return 1;
    }

    // Check if blockchain needs genesis block
    Block blocks[MAX_BLOCKS];
    int block_count;
    load_blockchain(blocks, &block_count);

    if (block_count == 0) {
        printf("Creating genesis block...\n");
        Block genesis = create_genesis();
        save_block(&genesis);
    }

    // Main application loop
    int running = 1;
    while (running) {
        if (is_logged_in()) {
            todo_menu();
            running = handle_todo();
        } else {
            auth_menu();
            running = handle_auth();
        }
    }

    return 0;
}
