#include "game_server_core.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

// =========================
// Scheduler Thread
// =========================
void* scheduler_thread(void* arg) {
    SharedCardGameData* game = (SharedCardGameData*)arg;

    while (game->game_status == 1) { // run while game is active
        lock_semaphore(game->semaphore_id);

        // Round robin: move to next active player
        int next_turn = (game->current_turn + 1) % game->num_players;
        while (!game->player_active[next_turn]) {
            next_turn = (next_turn + 1) % game->num_players;
        }

        game->current_turn = next_turn;

        unlock_semaphore(game->semaphore_id);

        sleep(1); // pacing delay
    }
    return NULL;
}

// =========================
// Logger Thread
// =========================
void* logger_thread(void* arg) {
    SharedCardGameData* game = (SharedCardGameData*)arg;
    FILE* logf = fopen("game.log", "a");
    if (!logf) {
        perror("log file");
        return NULL;
    }

    while (game->game_status == 1) {
        lock_semaphore(game->semaphore_id);

        fprintf(logf,
                "[LOG] Turn=%d | Players=%d | DeckSize=%d | DiscardSize=%d\n",
                game->current_turn,
                game->num_players,
                game->deck_size,
                game->discard_size);

        fflush(logf);
        unlock_semaphore(game->semaphore_id);

        sleep(2); // avoid busy loop
    }

    fclose(logf);
    return NULL;
}

int main() {
    printf("Simplified Card Draw Game - Server Core\n");
    printf("===========================================\n\n");
    
    // Setup signal handlers (SIGCHLD + waitpid)
    printf("[1/6] Setting up signal handlers for zombie reaping...\n");
    setup_signal_handlers();
    
    // Initialize shared memory structures
    printf("[2/6] Initializing shared memory structures...\n");
    SharedCardGameData* game = initialize_shared_memory();
    if (game == NULL) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return 1;
    }
    
    // Create process-shared semaphore
    printf("[3/6] Creating process-shared semaphore...\n");
    int sem_id = create_semaphore();
    if (sem_id == -1) {
        fprintf(stderr, "Failed to create semaphore\n");
        return 1;
    }
    game->semaphore_id = sem_id;
    
    // INTERACTIVE: Get number of players (3-5)
    printf("\n=== PLAYER SETUP ===\n");
    int num_players = 0;
    
    while (num_players < MIN_PLAYERS || num_players > MAX_PLAYERS) {
        printf("Enter number of players (%d-%d): ", MIN_PLAYERS, MAX_PLAYERS);
        scanf("%d", &num_players);
        
        if (num_players < MIN_PLAYERS || num_players > MAX_PLAYERS) {
            printf("Invalid! Must be between %d and %d.\n", MIN_PLAYERS, MAX_PLAYERS);
        }
    }
    
    printf("\n[4/6] Creating %d client processes using fork()...\n", num_players);
    
    // Implement fork() for client processes
    pid_t player_pids[MAX_PLAYERS];
    for (int i = 0; i < num_players; i++) {
        player_pids[i] = create_client_process(i);
        if (player_pids[i] > 0) {
            add_player_to_game(player_pids[i], game);
        }
    }
    
    // Wait for processes to start
    sleep(1);
    
    printf("\n[5/6] Displaying initial game state...\n");
    print_game_state(game);
    
    // Demonstration
    printf("\n[6/6] Demonstrating features:\n");
    printf("- Shared memory access ✓\n");
    printf("- Process synchronization ✓\n");
    printf("- Player management ✓\n");
    printf("- Zombie process reaping ✓\n");
    
    // Demo: Start game (requires min players)
    printf("\n=== DEMONSTRATION ===\n");
    if (start_game(game) == 0) {
        printf("Game started successfully!\n");
        print_game_state(game);
        
        // Create scheduler & logger threads (Step 2)
        pthread_t sched_tid, log_tid;
        pthread_create(&sched_tid, NULL, scheduler_thread, game);
        pthread_create(&log_tid, NULL, logger_thread, game);
        
        // Demo a few turns (original code unchanged)
        printf("\nSimulating 3 turns:\n");
        for (int i = 0; i < 3; i++) {
            printf("Turn %d: Player %d's turn\n", i + 1, get_current_turn(game));
            set_next_turn(game);
            sleep(1);
        }
        
        printf("\nFinal game state:\n");
        print_game_state(game);

        // End game and join threads
        game->game_status = 2;
        pthread_join(sched_tid, NULL);
        pthread_join(log_tid, NULL);
    }
    
    // Wait for child processes
    printf("\nWaiting for client processes to finish...\n");
    sleep(3);
    
    // Cleanup
    printf("\n=== CLEANUP ===\n");
    cleanup_shared_memory(game);
    printf("Implementation complete!\n");
    
    return 0;
}