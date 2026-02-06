# Blockchain Todo Application

A command-line blockchain application that combines a todo task manager with a cryptocurrency-style transaction ledger and mining simulation. Supports two ledger models (UTXO and Account-Balance) and three mining modes (Solo, Pool, Cloud).

## Prerequisites

- GCC compiler
- Make

No external libraries required — the application uses a pure-C SHA-256 implementation with no dependency on OpenSSL or any other third-party library.

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential
```

## Compilation

```bash
make
```

This produces the `blockchain` executable. The build uses `-Wall -Wextra` and compiles with zero warnings.

## Running

```bash
./blockchain
```

On first run the application creates a `data/` directory and a genesis block automatically.

## Fresh Start

To clear all data and start over:
```bash
rm -f data/blockchain.txt data/users.txt data/config.txt
```

## Usage Guide

### Authentication

When the application starts you see the auth menu:
```
=== Blockchain App ===
1. Login
2. Register
3. Exit
```

- **Register** creates a new user account. Passwords are SHA-256 hashed before storage. On registration the user receives **100.00 coins** via an automatic faucet coinbase block.
- **Login** authenticates with an existing account.

### Main Menu (after login)

```
=== Blockchain App (alice) ===
1. Todo Tasks
2. Wallet
3. Mining
4. Settings
5. Verify Blockchain
6. Logout
```

### 1. Todo Tasks

```
1. Add Task     — Create a new task (mined as a TX_TASK block with PoW)
2. View Tasks   — Display your tasks with completion status
3. Complete Task — Mark a task as done (creates a new block)
0. Back
```

Tasks are stored immutably on the blockchain. Each task operation (add/complete) requires mining a block with Proof of Work at the current difficulty.

### 2. Wallet

```
1. Send Coins          — Transfer coins to another user
2. View Balance        — Show current balance
3. Transaction History — List all incoming/outgoing transactions
4. View UTXOs          — Show unspent transaction outputs (UTXO model only)
0. Back
```

The wallet behavior adapts to the active ledger model:
- **UTXO model**: Transfers consume unspent outputs and produce new ones (with change)
- **Account-Balance model**: Transfers debit the sender and credit the receiver directly

### 3. Mining

```
1. Solo Mine          — Mine a block, receive full 50.00 coin reward
2. Pool Mining Setup  — Configure a mining pool with multiple miners and hashrate shares
3. Pool Mine          — Mine as a pool, see reward distribution (off-chain)
4. Cloud Mining Setup — Create a cloud contract with rental and maintenance fees
5. Cloud Mine         — Mine via cloud, see fee deductions (off-chain)
6. Mining Stats       — View your mining history and total rewards
0. Back
```

### 4. Settings

```
1. Switch Ledger Model  — Toggle between UTXO and Account-Balance
2. Set Mining Difficulty — Set difficulty from 1 to 6 (number of leading hex zeros)
0. Back
```

### 5. Verify Blockchain

Checks the entire chain for:
- Transaction hash integrity
- Block hash integrity
- Proof of Work validity (for blocks with difficulty > 0)
- Chain linkage (each block's prev_hash matches the previous block's hash)

## Switching Between Ledger Models

1. Log in to any account
2. Go to **Settings** (option 4)
3. Select **Switch Ledger Model** (option 1)
4. The model toggles between UTXO and Account-Balance

The setting is saved to `data/config.txt` and persists across sessions. Both models derive balances from the same blockchain — switching models does not affect stored data.

**UTXO model** tracks individual unspent outputs. Transfers select specific UTXOs as inputs and produce new outputs (payment + change). Double-spend protection prevents reusing spent outputs.

**Account-Balance model** maintains an in-memory account table rebuilt from the chain on startup. Transfers simply check that the sender's balance is sufficient.

## Testing Mining Simulations

### Solo Mining
1. Go to **Mining** → **Solo Mine**
2. A coinbase block is mined with the current difficulty
3. You receive the full 50.00 coin block reward

### Pool Mining
1. Go to **Mining** → **Pool Mining Setup**
2. Add miners with hashrate shares (e.g., alice:3, bob:2, charlie:5)
3. Press Enter with empty name to finish setup
4. Go to **Pool Mine** — one block is mined, and the reward distribution is displayed showing each miner's share after the pool fee (default 2%)

### Cloud Mining
1. Go to **Mining** → **Cloud Mining Setup**
2. A contract is created with rental cost (10.00/block) and maintenance fee (5.00/block)
3. Go to **Cloud Mine** — a block is mined and a summary shows gross reward, deductions, and net earnings

### Adjusting Difficulty
Go to **Settings** → **Set Mining Difficulty** and choose 1–6. Higher values require more leading zeros in the block hash, increasing mining time exponentially. Default is 3 (~4000 attempts, sub-second). Difficulty 5+ may take several seconds.

## Data Files

| File | Format | Purpose |
|------|--------|---------|
| `data/blockchain.txt` | 17 pipe-delimited fields per line | Blockchain with transactions, mining data |
| `data/users.txt` | `username:password_hash` per line | User credentials |
| `data/config.txt` | `key=value` per line | Ledger model and difficulty settings |

## Project Structure

```
src/
├── main.c          — CLI menus and application loop
├── blockchain.h/c  — Block/Transaction structs, hashing, chain verification
├── sha256.h/c      — Pure-C SHA-256 (no external dependencies)
├── transaction.h/c — UTXO set, account-balance state, transaction creation/validation
├── mining.h/c      — PoW core, solo/pool/cloud mining
├── auth.h/c        — User registration, login, session management
├── todo.h/c        — Task management (add, view, complete)
└── storage.h/c     — File I/O for blockchain, users, and config
```

## Assumptions and Notes

- All monetary amounts are stored as `long long` integers in base units (100 base units = 1 coin) to avoid floating-point precision issues. Displayed with 2 decimal places.
- Each block contains exactly one transaction.
- Usernames and task text cannot contain `|`, `;`, or `,` characters (used as storage delimiters).
- Pool and cloud mining reward distributions are displayed off-chain (printed to screen). The on-chain block is a single coinbase transaction.
- The genesis block has difficulty 0 (no PoW required). All subsequent blocks require PoW.
- Account balances are derived from the blockchain and rebuilt on startup — there is no separate balance file.
