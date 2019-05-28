//
// Created by olliekrk on 28.05.19.
//

#ifndef KROLIKOLGIERD_ROLLERCOASTER_H
#define KROLIKOLGIERD_ROLLERCOASTER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

typedef struct Passenger {
    int passengerID;
    int trolleyID;
} Passenger;

typedef struct Trolley {
    int trolleyID;
    int loopsLeft;
    int passengersCount;
    Passenger *passengers;
} Trolley;

typedef enum CoasterEventType {
    PASSENGER_IN,
    PASSENGER_OUT,
    PASSENGER_START,
    PASSENGER_KILL,
    TROLLEY_CLOSE,
    TROLLEY_START,
    TROLLEY_STOP,
    TROLLEY_OPEN,
    TROLLEY_KILL
} CoasterEventType;

int TOTAL_PASSENGERS = -1;
int TOTAL_TROLLEYS = -1;
int TOTAL_CAPACITY = -1;
int TOTAL_LOOPS = -1;

pthread_t *T_PASSENGERS;
pthread_t *T_TROLLEYS;

Passenger *PASSENGERS;
Trolley *TROLLEYS;

pthread_cond_t *trolleys_cond;
pthread_mutex_t *trolleys_mutex;

pthread_mutex_t station_mutex = PTHREAD_MUTEX_INITIALIZER; // locked by currently server trolley
pthread_mutex_t passenger_mutex = PTHREAD_MUTEX_INITIALIZER; //
pthread_mutex_t trolley_empty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trolley_ready_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t trolley_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t trolley_ready_cond = PTHREAD_COND_INITIALIZER;

int station_trolleyID = -1;
int RUN = 1;

// MAIN FUNCTIONS HEADERS

void *passenger_role(void *);

void passenger_cleanup(int);

void *trolley_role(void *);

void initialize_coaster();

void start_coaster();

void finalize_coaster();

// TIME MEASUREMENT

typedef struct timeval tv;

void get_current_time(tv *buffer) {
    gettimeofday(buffer, NULL);
}

// UTILITY FUNCTIONS

void show_error(const char *message) {
    perror(message);
    exit(1);
}

void print_coaster_event(CoasterEventType event, int ID, int capacity) {
    tv time;
    get_current_time(&time);
    printf("[ %ld s %ld us ]: ", time.tv_sec, time.tv_usec);

    switch (event) {
        case PASSENGER_IN:
            printf("PASSENGER: %d IN, CURRENT LOAD: %d\n", ID, capacity);
            break;
        case PASSENGER_OUT:
            printf("PASSENGER: %d OUT, CURRENT LOAD: %d\n", ID, capacity);
            break;
        case PASSENGER_START:
            printf("PASSENGER: %d PRESSED START\n", ID);
            break;
        case PASSENGER_KILL:
            printf("PASSENGER HAS LEFT\n");
            break;
        case TROLLEY_CLOSE:
            printf("TROLLEY: %d DOOR CLOSED\n", ID);
            break;
        case TROLLEY_START:
            printf("TROLLEY: %d STARTED\n", ID);
            break;
        case TROLLEY_STOP:
            printf("TROLLEY: %d STOPPED\n", ID);
            break;
        case TROLLEY_OPEN:
            printf("TROLLEY: %d DOOR OPENED\n", ID);
            break;
        case TROLLEY_KILL:
            printf("TROLLEY: %d FINISHED ALL LOOPS\n", ID);
            break;
        default:
            show_error("Invalid coaster event");
    }
}

#endif //KROLIKOLGIERD_ROLLERCOASTER_H
