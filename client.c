#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "client.h"

int main() {
    srand(getpid());
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); exit(1); }

    game_state_t *state = mmap(NULL, sizeof(game_state_t),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) { perror("mmap"); exit(1); }

    int my_id = -1;

    // Safely assign a unique client ID
    pthread_mutex_lock(&state->lock);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!state->active[i]) {
            my_id = i;
            state->active[i] = 1;
            state->player_count++;
            break;
        }
    }
    pthread_mutex_unlock(&state->lock);

    if (my_id == -1) {
        printf("No available slots. Game full.\n");
        exit(1);
    }

    printf("Client %d joined the game\n", my_id);

    while (1) {
        pthread_mutex_lock(&state->lock);

        if (state->game_over) {
            if (state->winner_id == my_id) {
                printf("🎉 You won the game with score %d!\n", state->player_scores[my_id]);
            } else if (state->winner_id == -1) {
                printf("Game over. No winner — all players busted.\n");
            } else {
                printf("Game over. Player %d won with score %d.\n",
                       state->winner_id, state->player_scores[state->winner_id]);
            }
            pthread_mutex_unlock(&state->lock);
            break;
        }

        if (state->current_turn == my_id) {
            printf("It's my turn! (Client %d)\n", my_id);

            char choice;
            while (1) {
                printf("Enter D to draw or H to hold: ");
                scanf(" %c", &choice);

                if (choice == 'D' || choice == 'd') {
                    int card = rand() % 11 + 1;
                    state->player_scores[my_id] += card;
                    printf("You drew %d, new score: %d\n",
                           card, state->player_scores[my_id]);

                    if (state->player_scores[my_id] > 21) {
                        printf("You busted! Final score: %d\n", state->player_scores[my_id]);
                    }
                    break; // valid input handled
                } else if (choice == 'H' || choice == 'h') {
                    printf("You held. Current score: %d\n",
                           state->player_scores[my_id]);
                    break; // valid input handled
                } else {
                    printf("Invalid choice. Please enter D or H.\n");
                }
            }

            // Round increment
            state->rounds_played[my_id]++;

            // Instant win check
            if (state->player_scores[my_id] == 21) {
                printf("🎉 You hit 21! Instant win!\n");
                state->game_over = 1;
                state->winner_id = my_id;
            }

        } else {
            printf("Client %d waiting... current_turn=%d\n", my_id, state->current_turn);
            fflush(stdout);
        }

        pthread_mutex_unlock(&state->lock);
        sleep(1);
    }

    return 0;
}