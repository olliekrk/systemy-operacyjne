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
#define BELT_CAP_SEM 0
#define BELT_LOAD_SEM 1
#define GLOBAL_LOCK_SEM 2

// data structures
typedef enum event_type {
    TRUCK_ARRIVAL,
    TRUCK_LOADING,
    TRUCK_DEPARTURE,
    TRUCK_WAIT,
    LOADER_WAIT,
    ITEM_LOADED
} event_type;

typedef struct timeval tv;

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
    belt_item *items;
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
           "CURRENT LOAD: %d kg\tCURRENT CAP: %d items\n",
           event_type_name(event->type),
           event->time.tv_sec,
           event->time.tv_usec,
           event->pid,
           event->current_cb_load,
           event->current_cb_cap);
}

// for conveyor belt operations (cancerous)
belt_item *belt_item_clone(belt_item original) {
    belt_item *copy = malloc(sizeof(belt_item));
    copy->weight = original.weight;
    copy->time = original.time;
    copy->loader_pid = original.loader_pid;

    return copy;
}

belt_item belt_peek(conveyor_belt *belt) {
    return belt->items[0];
}

void belt_pop(conveyor_belt *belt) {
    if (belt->current_cap == 0) show_error("Attempted to pop an item from empty belt");

    belt_item popped;
    int i = belt->current_cap - 1;
    while (i > 0) {
        popped = belt->items[i - 1];
        belt->items[i - 1] = belt->items[i];
        i--;
    }

    belt->current_load -= popped.weight;
    belt->current_cap--;
}

void belt_push(conveyor_belt *belt, belt_item item) {
    belt->items[belt->current_cap] = item;
    belt->current_cap++;
    belt->current_load += item.weight;
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

void semaphore_load_truck(int sem_set_id, int item_weight) {
    struct sembuf ops[2];

    ops[0].sem_flg = 0;
    ops[0].sem_num = BELT_LOAD_SEM;
    ops[0].sem_op = item_weight;

    ops[1].sem_flg = 0;
    ops[1].sem_num = BELT_CAP_SEM;
    ops[1].sem_op = 1;

    if (semop(sem_set_id, ops, 2) == -1) show_error("Failed to load truck");
}

void semaphore_load_item(int sem_set_id, int item_weight) {
    struct sembuf ops[2];

    ops[0].sem_flg = 0;
    ops[0].sem_num = BELT_LOAD_SEM;
    ops[0].sem_op = -item_weight;

    ops[1].sem_flg = 0;
    ops[1].sem_num = BELT_CAP_SEM;
    ops[1].sem_op = -1;

    if (semop(sem_set_id, ops, 2) == -1) show_error("Failed to load truck");
}

void semaphore_lock_take(int sem_id) {

}

void semaphore_lock_release(int sem_id) {

}

#endif //CW7_UTILS