#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "client.h"

void *scheduler_thread(void *arg) {
    game_state_t *state = (game_state_t *)arg;
    int last_turn = -1;

    while (1) {
        pthread_mutex_lock(&state->lock);

        if (state->game_over) {
            if (state->winner_id == -1) {
                printf("Game over! No winner — all players busted.\n");
            } else {
                printf("Game over! Player %d wins with score %d\n",
                       state->winner_id, state->player_scores[state->winner_id]);
            }
            pthread_mutex_unlock(&state->lock);
            break;
        }

        int next_turn = -1;
        for (int i = 1; i <= MAX_PLAYERS; i++) {
            int candidate = (last_turn + i) % MAX_PLAYERS;
            if (state->active[candidate] &&
                state->rounds_played[candidate] < 4 &&
                state->player_scores[candidate] <= 21) {
                next_turn = candidate;
                break;
            }
        }

        if (next_turn != -1) {
            state->current_turn = next_turn;
            last_turn = next_turn;
            printf("Turn changed to Player %d\n", next_turn);
            fflush(stdout);
        } else {
            // All players finished or busted — determine winner
            int best_score = -1;
            int winner = -1;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (state->active[i] &&
                    state->rounds_played[i] == 4 &&
                    state->player_scores[i] <= 21) {
                    if (state->player_scores[i] > best_score) {
                        best_score = state->player_scores[i];
                        winner = i;
                    }
                }
            }
            state->game_over = 1;
            state->winner_id = winner;
        }

        pthread_mutex_unlock(&state->lock);
        sleep(3);
    }

    return NULL;
}

// Logger thread: writes scores to score.txt periodically
void *logger_thread(void *arg) {
    game_state_t *state = (game_state_t *)arg;

    while (1) {
        pthread_mutex_lock(&state->lock);

        FILE *fp = fopen("score.txt", "w");
        if (fp == NULL) {
            perror("fopen");
            pthread_mutex_unlock(&state->lock);
            break;
        }

        fprintf(fp, "Current Scores:\n");
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (state->active[i]) {
                fprintf(fp, "Player %d: %d (Rounds: %d)\n",
                        i, state->player_scores[i], state->rounds_played[i]);
            }
        }

        if (state->game_over) {
            if (state->winner_id == -1) {
                fprintf(fp, "Game over! No winner — all players busted.\n");
            } else {
                fprintf(fp, "Game over! Player %d wins with score %d\n",
                        state->winner_id, state->player_scores[state->winner_id]);
            }
            fclose(fp);
            pthread_mutex_unlock(&state->lock);
            break;
        }

        fclose(fp);
        pthread_mutex_unlock(&state->lock);

        sleep(2); // log every 2 seconds
    }

    return NULL;
}

int main() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); exit(1); }

    ftruncate(fd, sizeof(game_state_t));

    game_state_t *state = mmap(NULL, sizeof(game_state_t),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) { perror("mmap"); exit(1); }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->lock, &attr);

    // Enforce 3–5 players
    do {
        printf("Enter number of players (3-5): ");
        scanf("%d", &state->expected_players);
        if (state->expected_players < 3 || state->expected_players > 5) {
            printf("Invalid number of players. Please enter between 3 and 5.\n");
        }
    } while (state->expected_players < 3 || state->expected_players > 5);

    state->player_count = 0;
    state->current_turn = -1;
    state->game_over = 0;
    state->winner_id = -1;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->active[i] = 0;
        state->player_scores[i] = 0;
        state->rounds_played[i] = 0;
    }

    while (state->player_count < state->expected_players) {
        printf("Waiting for players... (%d/%d)\n", state->player_count, state->expected_players);
        fflush(stdout);
        sleep(1);
    }

    printf("Server running with shared memory only...\n");

    pthread_t scheduler, logger;
    pthread_create(&scheduler, NULL, scheduler_thread, state);
    pthread_create(&logger, NULL, logger_thread, state);

    pthread_join(scheduler, NULL);
    pthread_join(logger, NULL);

    return 0;
}