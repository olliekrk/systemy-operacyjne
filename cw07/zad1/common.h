#ifndef CW7_UTILS
#define CW7_UTILS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define SEM_SEED 88
#define BELT_SEED 99

#define NUMBER_OF_SEMAPHORES 3

// specific conveyor belt semaphores IDs
#define BELT_CAP_SEM 0 // capacity left on conveyor belt
#define BELT_LOAD_SEM 1 // load left on conveyor belt
#define GLOBAL_LOCK_SEM 2

// so that items arrays can be static
#define MAXIMUM_BELT_CAP 500

// data structures
typedef struct timeval tv;

typedef enum event_type {
    TRUCK_ARRIVAL,
    TRUCK_LOADING,
    TRUCK_DEPARTURE,
    TRUCK_WAIT,
    LOADER_WAIT,
    ITEM_LOADED
} event_type;

typedef struct belt_event {
    event_type type;
    tv time;
    pid_t pid; // PID for worker events
    int current_cb_cap; // cb = conveyor belt
    int current_cb_load;
} belt_event;

typedef struct belt_item {
    pid_t loader_pid;
    int weight;
    tv time;
} belt_item;

typedef struct conveyor_belt {
    int max_cap;
    int current_cap;
    int max_load;
    int current_load;
    belt_item items[MAXIMUM_BELT_CAP];
} conveyor_belt;

// utility
void show_error(const char *message) {
    perror(message);
    exit(1);
}

void get_current_time(struct timeval *buffer) {
    gettimeofday(buffer, NULL);
}

belt_event *generate_event(conveyor_belt *belt, event_type type) {
    belt_event *event = malloc(sizeof(belt_event));
    get_current_time(&event->time);
    event->pid = getpid();
    event->type = type;
    event->current_cb_cap = belt->current_cap;
    event->current_cb_load = belt->current_load;
    return event;
}

char *event_type_name(event_type type) {
    switch (type) {
        case TRUCK_ARRIVAL:
            return "TRUCK_ARRIVAL";
        case TRUCK_LOADING:
            return "TRUCK_LOADING";
        case TRUCK_DEPARTURE:
            return "TRUCK_DEPARTURE";
        case TRUCK_WAIT:
            return "TRUCK_WAIT";
        case LOADER_WAIT:
            return "LOADER_WAIT";
        case ITEM_LOADED:
            return "ITEM_LOADED";
    }
}

void print_event(belt_event *event) {
    printf("EVENT: %s\n\t"
           "TIME: %ld s\t%ld ms\n\t"
           "PID: %d\n\t"
           "CURRENT BELT LOAD: %d kg\tCURRENT BELT CAP: %d items\n",
           event_type_name(event->type),
           event->time.tv_sec,
           event->time.tv_usec,
           event->pid,
           event->current_cb_load,
           event->current_cb_cap);
}

// for conveyor belt operations (cancerous)
belt_item belt_peek(conveyor_belt *belt) {
    return belt->items[0];
}

void belt_pop(conveyor_belt *belt) {
    if (belt->current_cap == 0) show_error("Attempted to pop an item from empty belt");

    belt_item popped = belt->items[0];
    for (int i = belt->current_cap - 1; i > 0; i--) {
        popped = belt->items[i - 1];
        belt->items[i - 1] = belt->items[i];
    }

    belt->current_load -= popped.weight;
    belt->current_cap--;
}

void belt_push(conveyor_belt *belt, belt_item item) {
    belt->items[belt->current_cap] = item;
    belt->current_load += item.weight;
    belt->current_cap += 1;
}

// for generating keys
key_t receive_key(int id) {
    char *home_path = getenv("HOME");
    if (!home_path) show_error("$HOME is unavailable");

    key_t generated_key = ftok(home_path, id);
    if (generated_key == -1) show_error("Unable to generate key.");

    return generated_key;
}

key_t receive_sem_key() {
    return receive_key(SEM_SEED);
}

key_t receive_belt_key() {
    return receive_key(BELT_SEED);
}

// for managing semaphores
void semaphore_set(int sem_set_id, int sem_id, int value) {
    if (semctl(sem_set_id, sem_id, SETVAL, value) == -1)
        show_error("Failed to set semaphore value");
}

int semaphore_get(int sem_set_id, int sem_id) {
    return semctl(sem_set_id, sem_id, GETVAL);
}

int semaphore_item_to_truck(int sem_set_id, int item_weight) {
    struct sembuf ops[3];

    ops[0].sem_op = item_weight;
    ops[0].sem_num = BELT_LOAD_SEM;
    ops[0].sem_flg = 0;

    ops[1].sem_op = 1;
    ops[1].sem_num = BELT_CAP_SEM;
    ops[1].sem_flg = 0;

    ops[2].sem_op = -1;
    ops[2].sem_num = GLOBAL_LOCK_SEM;
    ops[2].sem_flg = 0;

    return semop(sem_set_id, ops, 3);
}

int semaphore_item_to_belt(int sem_set_id, int item_weight) {
    struct sembuf ops[3];

    ops[0].sem_op = -item_weight;
    ops[0].sem_num = BELT_LOAD_SEM;
    ops[0].sem_flg = IPC_NOWAIT;

    ops[1].sem_op = -1;
    ops[1].sem_num = BELT_CAP_SEM;
    ops[1].sem_flg = IPC_NOWAIT;

    ops[2].sem_op = -1;
    ops[2].sem_num = GLOBAL_LOCK_SEM;
    ops[2].sem_flg = IPC_NOWAIT;

    return semop(sem_set_id, ops, 3);
}

int semaphore_lock_take(int sem_set_id) {
    struct sembuf op[1];
    op[0].sem_op = -1;
    op[0].sem_num = GLOBAL_LOCK_SEM;
    op[0].sem_flg = 0;

    return semop(sem_set_id, op, 1);
}

int semaphore_lock_release(int sem_set_id) {
    struct sembuf op[1];
    op[0].sem_op = 1;
    op[0].sem_num = GLOBAL_LOCK_SEM;
    op[0].sem_flg = 0;

    return semop(sem_set_id, op, 1);
}

#endif //CW7_UTILS