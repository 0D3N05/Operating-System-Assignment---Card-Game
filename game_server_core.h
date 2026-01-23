#ifndef GAME_SERVER_CORE_H
#define GAME_SERVER_CORE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Game constants from requirements
#define MAX_PLAYERS 5
#define MIN_PLAYERS 3
#define MAX_CARDS_PER_PLAYER 10
#define DECK_SIZE 52
#define INITIAL_CARDS 5

// Shared memory structure
typedef struct {
    // Game state
    int game_status;           // 0=waiting, 1=active, 2=ended
    int num_players;           // Current number of players (3-5)
    int current_turn;          // Whose turn it is
    
    // Card deck state
    int deck[DECK_SIZE];
    int deck_size;
    int discard_pile[DECK_SIZE];
    int discard_size;
    
    // Player states
    pid_t player_pids[MAX_PLAYERS];        // Process IDs from fork()
    int player_active[MAX_PLAYERS];        // 1=active, 0=inactive
    int player_scores[MAX_PLAYERS];        // Scores
    
    // Player cards
    int player_hand_size[MAX_PLAYERS];
    int player_hands[MAX_PLAYERS][MAX_CARDS_PER_PLAYER];
    
    // Synchronization
    int semaphore_id;
    int turn_completed;
    
    // Randomness (server-side only)
    unsigned int random_seed;
} SharedCardGameData;

// FUNCTION DECLARATIONS 

// 1. Process management (fork(), SIGCHLD, waitpid())
void setup_signal_handlers(void);
void sigchld_handler(int sig);
pid_t create_client_process(int player_id);

// 2. Shared memory structures
SharedCardGameData* initialize_shared_memory(void);
void cleanup_shared_memory(SharedCardGameData* data);

// 3. Process-shared mutexes/semaphores
int create_semaphore(void);
void lock_semaphore(int sem_id);
void unlock_semaphore(int sem_id);

// 4. Game logic helpers 
void initialize_deck(SharedCardGameData* game);
void shuffle_deck(SharedCardGameData* game);
int draw_card(SharedCardGameData* game);

// 5. Player management
int add_player_to_game(pid_t pid, SharedCardGameData* game);
int remove_player_from_game(pid_t pid, SharedCardGameData* game);
int start_game(SharedCardGameData* game);

// 6. APIs for other modules 
int get_current_turn(SharedCardGameData* game);
void set_next_turn(SharedCardGameData* game);
void print_game_state(SharedCardGameData* game);

#endif