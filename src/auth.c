#include "auth.h"
#include "storage.h"
#include "crypto.h"
#include <stdio.h>
#include <string.h>

static char current_user[MAX_USERNAME_LEN] = "";

int register_user(const char *username, const char *password) {
    // Validate input
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Username and password cannot be empty.\n");
        return 0;
    }
    if (strlen(username) >= MAX_USERNAME_LEN) {
        printf("Username too long (max %d characters).\n", MAX_USERNAME_LEN - 1);
        return 0;
    }

    // Check if username already exists
    User users[MAX_USERS];
    int count;
    load_users(users, &count);

    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("Username already exists.\n");
            return 0;
        }
    }

    // Create new user
    User new_user;
    strncpy(new_user.username, username, MAX_USERNAME_LEN - 1);
    new_user.username[MAX_USERNAME_LEN - 1] = '\0';
    sha256(password, new_user.password_hash);

    if (!save_user(&new_user)) {
        printf("Failed to save user.\n");
        return 0;
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
