#ifndef CLIENT_H
#define CLIENT_H

#define MAX_PLAYERS 5
#define SHM_NAME "/game_shm"

typedef struct {
    int current_turn;
    int player_scores[MAX_PLAYERS];
    int active[MAX_PLAYERS];
    int player_count;
    int expected_players;
    int rounds_played[MAX_PLAYERS];
    int game_over;
    int winner_id;
    pthread_mutex_t lock;
} game_state_t;

#endif