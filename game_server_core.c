#include "game_server_core.h"

// Signal handler
void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[SIGCHLD] Child %d terminated\n", pid);
    }
}

void setup_signal_handlers(void) {
    signal(SIGCHLD, sigchld_handler);
    printf("Signal handlers configured\n");
}

// Shared memory with IPC_PRIVATE
SharedCardGameData* initialize_shared_memory(void) {
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedCardGameData), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("shmget");
        return NULL;
    }
    
    SharedCardGameData* data = (SharedCardGameData*)shmat(shmid, NULL, 0);
    
    if (data == (void*)-1) {
        perror("shmat");
        return NULL;
    }
    
    memset(data, 0, sizeof(SharedCardGameData));
    data->random_seed = time(NULL);
    srand(data->random_seed);
    
    printf("Shared memory ready (ID: %d)\n", shmid);
    return data;
}

void cleanup_shared_memory(SharedCardGameData* data) {
    if (data) {
        shmdt(data);
        printf("Shared memory cleaned\n");
    }
}

// Semaphore with IPC_PRIVATE
int create_semaphore(void) {
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    
    if (semid == -1) {
        perror("semget");
        return -1;
    }
    
    union semun { int val; } arg;
    arg.val = 1;
    
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        return -1;
    }
    
    printf("Semaphore ready (ID: %d)\n", semid);
    return semid;
}

void lock_semaphore(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void unlock_semaphore(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

// Rest of your functions remain the same...
pid_t create_client_process(int player_id) {
    pid_t pid = fork();
    if (pid == 0) {
        printf("[CLIENT %d] PID: %d\n", player_id, getpid());
        sleep(2);
        exit(0);
    }
    return pid;
}

int add_player_to_game(pid_t pid, SharedCardGameData* game) {
    if (game->num_players >= MAX_PLAYERS) return -1;
    int idx = game->num_players;
    game->player_pids[idx] = pid;
    game->player_active[idx] = 1;
    game->player_scores[idx] = 0;
    game->num_players++;
    printf("[GAME] Player %d added\n", idx);
    return idx;
}

void initialize_deck(SharedCardGameData* game) {
    for (int i = 0; i < DECK_SIZE; i++) game->deck[i] = i;
    game->deck_size = DECK_SIZE;
}

void shuffle_deck(SharedCardGameData* game) {
    for (int i = game->deck_size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = game->deck[i];
        game->deck[i] = game->deck[j];
        game->deck[j] = temp;
    }
}

int start_game(SharedCardGameData* game) {
    if (game->num_players < MIN_PLAYERS) return -1;
    game->game_status = 1;
    game->current_turn = 0;
    initialize_deck(game);
    shuffle_deck(game);
    printf("[GAME] Started with %d players\n", game->num_players);
    return 0;
}

int get_current_turn(SharedCardGameData* game) {
    return game->current_turn;
}

void set_next_turn(SharedCardGameData* game) {
    game->current_turn = (game->current_turn + 1) % game->num_players;
}

void print_game_state(SharedCardGameData* game) {
    printf("\n=== GAME STATE ===\n");
    printf("Players: %d | Turn: %d\n", game->num_players, game->current_turn);
    for (int i = 0; i < game->num_players; i++) {
        printf("Player %d: PID=%d, Score=%d\n", i, game->player_pids[i], game->player_scores[i]);
    }
}