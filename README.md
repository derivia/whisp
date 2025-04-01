# Whisp 🌐

*A minimal, thread-safe real-time chat server and client written in C99.*

## Description

Whisp is a local-network group chat system where users can:

- Register/login.
- Create, join, leave, or delete chat groups.
- Send real-time messages.

Built with C99, pthreads and SQLite3 for simplicity and performance.

## Features

- Auth System
    - Password storage.
    - Username uniqueness enforcement.

- Group Chats
    - Create/delete groups (creator privileges).
    - Dynamic join/leave with broadcast notifications.

- Concurrency
    - Thread-per-client model.
    - Mutex-protected shared data (groups, user lists).

- No Dependencies
    - Pure C99 and SQLite3 (embedded).

## Usage

### 1. Build

```sh
# Clone and build (requires SQLite3)
git clone https://github.com/derivia/whisp
cd whisp
make           # Builds server + client
```

### 2. Run the Server

```sh
./whisp_server  # Listens on port 6969 by default
```

### 3. Run Clients

```sh
./whisp_client <server_ip> [port] # Connects to the server
```

Client commands:
- `register <user> <pass>`
- `login <user> <pass>`
- `create <group>`
- `enter <group>`
- `leave <group>`
- `delete <group>`

## Technical Design

### Server

- Thread Pool: Handles clients concurrently.
- Mutexes: Protect `groups_db` and `active_users`.
- SQLite: Stores `(username, password)` pairs.

### Client

- Non-blocking I/O (threads).
- Auto-reconnects on failure.

## Limitations

- No message history (ephemeral chats).
- No encryption (for now; local network only).

## License

[MIT](./LICENSE)
