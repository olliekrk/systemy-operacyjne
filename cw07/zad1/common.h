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

// weight related
#define MAX_ITEM_WEIGHT 10
#define DEFAULT_ITEM_WEIGHT 3
#define DEFAULT_CYCLE 50

#define SEM_SEED 88
#define BELT_SEED 99

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

typedef struct factory_event {
    event_type type;
    tv time;
    pid_t pid; // PID for worker events
    int current_cb_cap; // cb = conveyor belt
    int current_cb_load;
} factory_event;

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

factory_event *generate_factory_event(conveyor_belt *belt, event_type type) {
    factory_event *event = malloc(sizeof(factory_event));
    get_current_time(&event->time);
    event->pid = getpid();
    event->type = type;
    event->current_cb_cap = belt->current_cap;
    event->current_cb_load = belt->current_load;
    return event;
}


// for conveyor belt operations
belt_item belt_pop(conveyor_belt *belt) {

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
void set_semaphore(int sem_set_id, int sem_id, int value) {
    if (semctl(sem_set_id, sem_id, SETVAL, value) == -1)
        show_error("Failed to set semaphore value");
}

#endif //CW7_UTILS