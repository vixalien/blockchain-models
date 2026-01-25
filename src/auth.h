#ifndef AUTH_H
#define AUTH_H

#define MAX_USERNAME_LEN 50
#define HASH_LEN 65
#define MAX_USERS 100

typedef struct User {
    char username[MAX_USERNAME_LEN];
    char password_hash[HASH_LEN];
} User;

// Register a new user (returns 1 on success, 0 on failure)
int register_user(const char *username, const char *password);

// Login user (returns 1 on success, 0 on failure)
int login(const char *username, const char *password);

// Logout current user
void logout(void);

// Check if a user is logged in
int is_logged_in(void);

// Get current logged in username
const char* get_current_user(void);

#endif
