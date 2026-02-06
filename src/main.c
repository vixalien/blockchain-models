#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"
#include "todo.h"
#include "storage.h"
#include "blockchain.h"
#include "transaction.h"
#include "mining.h"

#define MAX_INPUT 256

static void get_input(const char *prompt, char *buffer, int max_len) {
    printf("%s", prompt);
    if (fgets(buffer, max_len, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

static int get_int_input(const char *prompt) {
    char buffer[32];
    get_input(prompt, buffer, sizeof(buffer));
    return atoi(buffer);
}

static long long get_amount_input(const char *prompt) {
    char buffer[32];
    get_input(prompt, buffer, sizeof(buffer));
    double val = atof(buffer);
    return (long long)(val * 100 + 0.5);
}

/* ---- Auth menu ---- */

static void auth_menu(void) {
    printf("\n=== Blockchain App ===\n");
    printf("1. Login\n");
    printf("2. Register\n");
    printf("3. Exit\n");
    printf("======================\n");
}

static int handle_auth(void) {
    int choice = get_int_input("Choose option: ");
    char username[MAX_USERNAME_LEN];
    char password[MAX_INPUT];

    switch (choice) {
        case 1:
            get_input("Username: ", username, MAX_USERNAME_LEN);
            get_input("Password: ", password, MAX_INPUT);
            login(username, password);
            break;
        case 2:
            get_input("Username: ", username, MAX_USERNAME_LEN);
            get_input("Password: ", password, MAX_INPUT);
            register_user(username, password);
            break;
        case 3:
            printf("Goodbye!\n");
            return 0;
        default:
            printf("Invalid option.\n");
    }
    return 1;
}

/* ---- Main menu (after login) ---- */

static void main_menu(void) {
    printf("\n=== Blockchain App (%s) ===\n", get_current_user());
    printf("1. Todo Tasks\n");
    printf("2. Wallet\n");
    printf("3. Mining\n");
    printf("4. Settings\n");
    printf("5. Verify Blockchain\n");
    printf("6. Logout\n");
    printf("==============================\n");
}

/* ---- Todo submenu ---- */

static void todo_submenu(void) {
    int running = 1;
    while (running) {
        printf("\n=== Todo Tasks ===\n");
        printf("1. Add Task\n");
        printf("2. View Tasks\n");
        printf("3. Complete Task\n");
        printf("0. Back\n");
        printf("==================\n");

        int choice = get_int_input("Choose option: ");
        char task[MAX_TASK_LEN];

        switch (choice) {
            case 1:
                get_input("Enter task: ", task, MAX_TASK_LEN);
                add_task(task);
                break;
            case 2:
                view_tasks();
                break;
            case 3:
                view_tasks();
                complete_task(get_int_input("Enter task number to complete: "));
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("Invalid option.\n");
        }
    }
}

/* ---- Wallet submenu ---- */

static void wallet_submenu(void) {
    int running = 1;
    while (running) {
        LedgerModel model;
        int difficulty;
        load_config(&model, &difficulty);

        printf("\n=== Wallet ===\n");
        printf("1. Send Coins\n");
        printf("2. View Balance\n");
        printf("3. Transaction History\n");
        if (model == LEDGER_UTXO)
            printf("4. View UTXOs\n");
        printf("0. Back\n");
        printf("==============\n");

        int choice = get_int_input("Choose option: ");

        Block blocks[MAX_BLOCKS];
        int block_count;

        switch (choice) {
            case 1: {
                char receiver[MAX_USERNAME_LEN];
                get_input("Recipient username: ", receiver, MAX_USERNAME_LEN);
                long long amount = get_amount_input("Amount (e.g., 25.00): ");

                if (amount <= 0) {
                    printf("Amount must be positive.\n");
                    break;
                }

                load_blockchain(blocks, &block_count);

                if (model == LEDGER_UTXO) {
                    UTXOSet set;
                    build_utxo_set(blocks, block_count, &set);

                    Transaction tx;
                    if (!create_utxo_transfer(&tx, get_current_user(), receiver, amount, &set)) {
                        break;
                    }
                    if (!validate_utxo_tx(&tx, &set)) {
                        break;
                    }

                    Block block = create_block(block_count, &tx, get_current_user(),
                                               difficulty, blocks[block_count - 1].hash);
                    printf("Mining transfer block...\n");
                    mine_block(&block);

                    if (save_block(&block)) {
                        printf("Sent %.2f coins to %s!\n", amount / 100.0, receiver);
                    }
                } else {
                    AccountState state;
                    rebuild_account_state(blocks, block_count, &state);

                    Transaction tx;
                    create_account_transfer_tx(&tx, get_current_user(), receiver, amount);

                    if (!validate_account_tx(&tx, &state)) {
                        break;
                    }

                    Block block = create_block(block_count, &tx, get_current_user(),
                                               difficulty, blocks[block_count - 1].hash);
                    printf("Mining transfer block...\n");
                    mine_block(&block);

                    if (save_block(&block)) {
                        printf("Sent %.2f coins to %s!\n", amount / 100.0, receiver);
                    }
                }
                break;
            }

            case 2: {
                load_blockchain(blocks, &block_count);

                if (model == LEDGER_UTXO) {
                    UTXOSet set;
                    build_utxo_set(blocks, block_count, &set);
                    long long bal = get_utxo_balance(&set, get_current_user());
                    printf("Balance (UTXO): %.2f coins\n", bal / 100.0);
                } else {
                    AccountState state;
                    rebuild_account_state(blocks, block_count, &state);
                    long long bal = get_account_balance(&state, get_current_user());
                    printf("Balance (Account): %.2f coins\n", bal / 100.0);
                }
                break;
            }

            case 3: {
                load_blockchain(blocks, &block_count);
                const char *user = get_current_user();

                printf("\n=== Transaction History ===\n");
                int shown = 0;
                for (int i = 0; i < block_count; i++) {
                    Transaction *tx = &blocks[i].tx;
                    if (tx->type == TX_TASK) continue;

                    if (strcmp(tx->sender, user) == 0 || strcmp(tx->receiver, user) == 0) {
                        const char *type_str = tx->type == TX_COINBASE ? "COINBASE" : "TRANSFER";
                        printf("Block %d [%s]: ", blocks[i].index, type_str);
                        if (tx->type == TX_COINBASE) {
                            printf("+%.2f coins (mined/faucet)\n", tx->amount / 100.0);
                        } else {
                            if (strcmp(tx->sender, user) == 0) {
                                printf("-%.2f coins to %s\n", tx->amount / 100.0, tx->receiver);
                            } else {
                                printf("+%.2f coins from %s\n", tx->amount / 100.0, tx->sender);
                            }
                        }
                        shown++;
                    }
                }
                if (shown == 0) printf("No transactions found.\n");
                printf("===========================\n");
                break;
            }

            case 4: {
                if (model != LEDGER_UTXO) {
                    printf("UTXOs only available in UTXO ledger model.\n");
                    break;
                }
                load_blockchain(blocks, &block_count);
                UTXOSet set;
                build_utxo_set(blocks, block_count, &set);
                const char *user = get_current_user();

                printf("\n=== Your UTXOs ===\n");
                int shown = 0;
                for (int i = 0; i < set.count; i++) {
                    if (strcmp(set.utxos[i].owner, user) == 0) {
                        printf("  %.2f coins (tx: %.8s... idx: %d)\n",
                               set.utxos[i].amount / 100.0,
                               set.utxos[i].tx_hash,
                               set.utxos[i].output_index);
                        shown++;
                    }
                }
                if (shown == 0) printf("  No UTXOs.\n");
                printf("==================\n");
                break;
            }

            case 0:
                running = 0;
                break;
            default:
                printf("Invalid option.\n");
        }
    }
}

/* ---- Mining submenu ---- */

static MiningPool g_pool;
static int g_pool_initialized = 0;
static CloudContract g_cloud;
static int g_cloud_initialized = 0;

static void mining_submenu(void) {
    int running = 1;
    while (running) {
        printf("\n=== Mining ===\n");
        printf("1. Solo Mine\n");
        printf("2. Pool Mining Setup\n");
        printf("3. Pool Mine\n");
        printf("4. Cloud Mining Setup\n");
        printf("5. Cloud Mine\n");
        printf("6. Mining Stats\n");
        printf("0. Back\n");
        printf("==============\n");

        int choice = get_int_input("Choose option: ");

        LedgerModel model;
        int difficulty;
        load_config(&model, &difficulty);

        Block blocks[MAX_BLOCKS];
        int block_count;

        switch (choice) {
            case 1:
                load_blockchain(blocks, &block_count);
                solo_mine(blocks, &block_count, get_current_user(), difficulty);
                break;

            case 2: {
                init_pool(&g_pool);
                g_pool_initialized = 1;
                printf("Pool initialized (fee: %d%%).\n", g_pool.pool_fee_percent);

                int adding = 1;
                while (adding) {
                    char miner_name[MAX_USERNAME_LEN];
                    get_input("Miner username (empty to finish): ", miner_name, MAX_USERNAME_LEN);
                    if (strlen(miner_name) == 0) {
                        adding = 0;
                    } else {
                        int share = get_int_input("Hashrate share: ");
                        if (add_pool_miner(&g_pool, miner_name, share)) {
                            printf("Added %s with share %d.\n", miner_name, share);
                        }
                    }
                }
                printf("Pool has %d miners, total shares: %d\n",
                       g_pool.miner_count, g_pool.total_shares);
                break;
            }

            case 3:
                if (!g_pool_initialized || g_pool.miner_count == 0) {
                    printf("Set up a pool first (option 2).\n");
                    break;
                }
                load_blockchain(blocks, &block_count);
                pool_mine(blocks, &block_count, &g_pool, difficulty);
                break;

            case 4:
                init_cloud_contract(&g_cloud, get_current_user());
                g_cloud_initialized = 1;
                printf("Cloud contract created.\n");
                printf("  Rental cost: %.2f coins/block\n", g_cloud.rental_cost / 100.0);
                printf("  Maintenance: %.2f coins/block\n", g_cloud.maintenance_fee / 100.0);
                break;

            case 5:
                if (!g_cloud_initialized) {
                    printf("Set up a cloud contract first (option 4).\n");
                    break;
                }
                load_blockchain(blocks, &block_count);
                cloud_mine(blocks, &block_count, &g_cloud, difficulty);
                break;

            case 6:
                load_blockchain(blocks, &block_count);
                print_mining_stats(blocks, block_count, get_current_user());
                break;

            case 0:
                running = 0;
                break;
            default:
                printf("Invalid option.\n");
        }
    }
}

/* ---- Settings submenu ---- */

static void settings_submenu(void) {
    int running = 1;
    while (running) {
        LedgerModel model;
        int difficulty;
        load_config(&model, &difficulty);

        printf("\n=== Settings ===\n");
        printf("1. Switch Ledger Model (current: %s)\n",
               model == LEDGER_UTXO ? "UTXO" : "Account-Balance");
        printf("2. Set Mining Difficulty (current: %d)\n", difficulty);
        printf("0. Back\n");
        printf("================\n");

        int choice = get_int_input("Choose option: ");

        switch (choice) {
            case 1:
                if (model == LEDGER_UTXO) {
                    model = LEDGER_ACCOUNT;
                    printf("Switched to Account-Balance model.\n");
                } else {
                    model = LEDGER_UTXO;
                    printf("Switched to UTXO model.\n");
                }
                save_config(model, difficulty);
                break;

            case 2: {
                int new_diff = get_int_input("Enter difficulty (1-6): ");
                if (new_diff < 1 || new_diff > 6) {
                    printf("Difficulty must be between 1 and 6.\n");
                } else {
                    difficulty = new_diff;
                    save_config(model, difficulty);
                    printf("Difficulty set to %d.\n", difficulty);
                }
                break;
            }

            case 0:
                running = 0;
                break;
            default:
                printf("Invalid option.\n");
        }
    }
}

/* ---- Main loop ---- */

static int handle_main_menu(void) {
    int choice = get_int_input("Choose option: ");

    switch (choice) {
        case 1:
            todo_submenu();
            break;
        case 2:
            wallet_submenu();
            break;
        case 3:
            mining_submenu();
            break;
        case 4:
            settings_submenu();
            break;
        case 5:
            verify_blockchain();
            break;
        case 6:
            logout();
            break;
        default:
            printf("Invalid option.\n");
    }
    return 1;
}

int main(void) {
    if (!init_data_files()) {
        fprintf(stderr, "Failed to initialize data files.\n");
        return 1;
    }

    /* Create genesis block if needed */
    Block blocks[MAX_BLOCKS];
    int block_count;
    load_blockchain(blocks, &block_count);

    if (block_count == 0) {
        printf("Creating genesis block...\n");
        Block genesis = create_genesis();
        save_block(&genesis);
    }

    int running = 1;
    while (running) {
        if (is_logged_in()) {
            main_menu();
            running = handle_main_menu();
        } else {
            auth_menu();
            running = handle_auth();
        }
    }

    return 0;
}
