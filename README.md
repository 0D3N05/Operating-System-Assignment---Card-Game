# Operating System Assignment - Card Game

A simplified multiplayer card draw game written in C for an Operating System assignment. The project demonstrates process creation, shared memory, semaphores, signal handling, and basic turn-based game coordination between multiple player processes.

## Overview

The program starts a server process that creates several child processes using `fork()`. Each child process represents one player. All players share the same game state through System V shared memory, while a System V semaphore protects critical sections such as drawing cards and updating scores.

Players take turns drawing cards from a shuffled 52-card deck. The game ends when the deck is empty, and each player prints their final score.

## Features

- Supports 3 to 5 players.
- Creates one child process per player using `fork()`.
- Stores shared game state in System V shared memory.
- Uses a process-shared semaphore to synchronize access to the deck and scores.
- Handles terminated child processes with `SIGCHLD` and `waitpid()`.
- Tracks player hands, scores, turns, and deck state.
- Provides a simple terminal-based turn flow.

## Project Structure

| File | Description |
| --- | --- |
| `main_server.c` | Main entry point. Sets up the server, asks for player count, creates player processes, starts the game, waits for children, and cleans up shared memory. |
| `game_server_core.c` | Core implementation for shared memory, semaphores, process creation, card drawing, turn handling, and game display. |
| `game_server_core.h` | Constants, shared data structure, includes, and function declarations. |
| `Makefile` | Build, run, and clean commands. |

## Requirements

This project uses Unix/POSIX APIs, including:

- `fork()`
- `signal()`
- `waitpid()`
- System V shared memory: `shmget()`, `shmat()`, `shmdt()`
- System V semaphores: `semget()`, `semctl()`, `semop()`

Because of this, build and run it on Linux, macOS with compatible System V IPC support, or Windows through WSL.

You also need:

- `gcc`
- `make`

## Build

```sh
make
```

This creates the executable:

```sh
part1_server
```

## Run

```sh
make run
```

Or run the executable directly:

```sh
./part1_server
```

When prompted, enter the number of players:

```text
Enter number of players (3-5):
```

Each player process waits for its turn. On a player's turn, press `Enter` to draw a card.

## Clean Build Files

```sh
make clean
```

## How The Game Works

1. The server configures signal handling for child process cleanup.
2. Shared memory is created for the game state.
3. A semaphore is created to protect shared data.
4. The user enters the number of players.
5. The server creates one child process for each player.
6. The deck is initialized and shuffled.
7. Players take turns drawing cards.
8. Drawing a card updates the player's hand and score.
9. When the deck is empty, the game status changes to ended.
10. Child processes exit and the server cleans up shared memory.

## Game Rules

- Each card is represented by a number from `0` to `51`.
- The value of the drawn card is added directly to the player's score.
- Turns move in player order from player `0` to the last player, then wrap back to player `0`.
- The game ends when no cards remain in the deck.

## Notes

- The game is intended as an OS concepts demonstration, not a full commercial card game.
- The program uses terminal input from multiple forked processes, so the console output may feel busy during gameplay.
- `INITIAL_CARDS` is defined in the header but is not currently used by the game flow.
