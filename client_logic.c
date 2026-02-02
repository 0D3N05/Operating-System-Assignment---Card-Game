#include "game_server_core.h"

void display_table(SharedCardGameData* game, int player_id) {
    // Simple CLI Dashboard
    printf("\033[H\033[J"); // Clear screen ANSI code
    printf("=== PLAYER %d (PID: %d) ===\n", player_id, getpid());
    printf("Your Score: %d\n", game->player_scores[player_id]);
    printf("Cards in Hand: ");
    for(int i=0; i < game->player_hand_size[player_id]; i++) {
        printf("[%d] ", game->player_hands[player_id][i]);
    }
    printf("\n\n");
    
    // Show other players
    printf("--- Opponents ---\n");
    for(int i=0; i < game->num_players; i++) {
        if(i != player_id) {
            printf("Player %d: Score %d\n", i, game->player_scores[i]);
        }
    }
    printf("-----------------\n");
}

void run_client(int player_id, SharedCardGameData* game) {
    printf("[CLIENT] Player %d ready.\n", player_id);

    while (game->game_status != 2) { // While game is active
        
        // 1. WAIT FOR TURN
        // Note: In a real efficient system we'd use condition variables, 
        // but for this assignment, simple polling with sleep is acceptable/common.
        if (get_current_turn(game) != player_id) {
            usleep(100000); // Sleep 100ms to save CPU
            continue;
        }

        // 2. MY TURN LOGIC
        display_table(game, player_id);
        printf("\n>>> YOUR TURN! Press ENTER to draw a card...");
        getchar(); // Wait for user input

        // 3. CRITICAL SECTION (Modifying Shared Memory)
        printf("Drawing card...\n");
        lock_semaphore(game->semaphore_id);
        
        int card = draw_card(game, player_id);
        
        unlock_semaphore(game->semaphore_id);
        // END CRITICAL SECTION

        if (card >= 0) {
            printf("You drew card #%d!\n", card);
        } else {
            printf("Could not draw card (Deck empty?).\n");
        }
        
        sleep(1); // Brief pause so user sees result

        // 4. END TURN
        // (If the Scheduler Thread is strictly required to manage turns, 
        // you might set a flag here like game->player_done = 1; 
        // For now, we manually pass turn to keep it playable).
        lock_semaphore(game->semaphore_id);
        set_next_turn(game);
        unlock_semaphore(game->semaphore_id);
        
        printf("Turn passed. Waiting...\n");
    }
    
    printf("Game Over! Final Score: %d\n", game->player_scores[player_id]);
    exit(0);
}