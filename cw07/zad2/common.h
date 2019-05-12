#ifndef CW7_UTILS
#define CW7_UTILS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

#define NUMBER_OF_SEMAPHORES 3

#define BELT_CAP_SEM_NAME "/beltCapacitySemaphore"
#define BELT_LOAD_SEM_NAME "/beltLoadSemaphore"
#define BELT_LOCK_SEM_NAME "/beltLockSemaphore"

#define BELT_NAME "/conveyorBelt"

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
    int current_cap;
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
        default:
            return "UNKNOWN";
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
    belt->current_cap ++;
}

#endif //CW7_UTILS