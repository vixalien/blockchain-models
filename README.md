# Blockchain Todo Application

A command-line todo list application that stores tasks on a blockchain for immutable record-keeping.

## Features

- User authentication (register/login with hashed passwords)
- Add, view, and complete tasks
- Blockchain-based storage ensuring data integrity
- Verify blockchain to detect tampering
- Per-user task isolation

## Prerequisites

- GCC compiler
- OpenSSL development libraries

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential libssl-dev
```

On Fedora/RHEL:
```bash
sudo dnf install gcc openssl-devel
```

## Compilation

```bash
make
```

## Usage

Run the application:
```bash
./todo
```

### Authentication Menu
1. **Login** - Log in with existing credentials
2. **Register** - Create a new account
3. **Exit** - Close the application

### Todo Menu (after login)
1. **Add Task** - Create a new task
2. **View Tasks** - Display all your tasks with completion status
3. **Complete Task** - Mark a task as done
4. **Verify Blockchain** - Check blockchain integrity
5. **Logout** - Return to authentication menu

## Data Storage

- `data/users.txt` - User credentials (username:password_hash)
- `data/blockchain.txt` - Serialized blockchain (index|timestamp|task|username|completed|prev_hash|hash)

## Blockchain Structure

Each block contains:
- Index (position in chain)
- Timestamp (Unix timestamp)
- Task description
- Username of task owner
- Completion status (0 or 1)
- Previous block hash (SHA-256)
- Current block hash (SHA-256)

## Security

- Passwords are hashed using SHA-256 before storage
- Each block's hash is computed from all its fields
- Blockchain verification detects any tampering with stored data

## Clean Up

```bash
make clean
```

This removes the compiled binary and data directory.
