//
// Created by olliekrk on 28.05.19.
//

#include "rollercoaster.h"

int main(int argc, char **argv) {
    if (argc < 5)
        show_error("Invalid number of arguments!\n"
                   "<TOTAL_PASSENGERS> <TOTAL_TROLLEYS> <TOTAL_CAPACITY> <TOTAL_LOOPS>");

    TOTAL_PASSENGERS = strtol(argv[1], NULL, 10);
    TOTAL_TROLLEYS = strtol(argv[2], NULL, 10);
    TOTAL_CAPACITY = strtol(argv[3], NULL, 10);
    TOTAL_LOOPS = strtol(argv[4], NULL, 10);

    if (TOTAL_PASSENGERS <= 0 || TOTAL_TROLLEYS <= 0 || TOTAL_CAPACITY <= 0 || TOTAL_LOOPS <= 0)
        show_error("Non-positive numbers were passed as arguments!");

    if (TOTAL_TROLLEYS * TOTAL_CAPACITY > TOTAL_PASSENGERS)
        show_error("Not enough passengers to fill the coaster");

    initialize_coaster();

    start_coaster();

    finalize_coaster();

    return 0;
}

void initialize_coaster() {
    T_PASSENGERS = calloc(TOTAL_PASSENGERS, sizeof(pthread_t));
    T_TROLLEYS = calloc(TOTAL_TROLLEYS, sizeof(pthread_t));

    PASSENGERS = calloc(TOTAL_PASSENGERS, sizeof(Passenger));
    TROLLEYS = calloc(TOTAL_TROLLEYS, sizeof(Trolley));

    trolleys_cond = calloc(TOTAL_TROLLEYS, sizeof(pthread_cond_t));
    trolleys_mutex = calloc(TOTAL_TROLLEYS, sizeof(pthread_mutex_t));

    if (!(T_PASSENGERS && T_TROLLEYS && PASSENGERS && TROLLEYS && trolleys_cond && trolleys_mutex))
        show_error("Memory allocation error during initialization!");

    for (int i = 0; i < TOTAL_PASSENGERS; i++) {
        PASSENGERS[i].passengerID = i;
        PASSENGERS[i].trolleyID = -1;
    }

    for (int i = 0; i < TOTAL_TROLLEYS; i++) {
        TROLLEYS[i].trolleyID = i;
        TROLLEYS[i].passengersCount = 0;
        TROLLEYS[i].loopsLeft = TOTAL_LOOPS;
        TROLLEYS[i].passengers = calloc(TOTAL_CAPACITY, sizeof(Passenger));
        pthread_mutex_init(&trolleys_mutex[i], NULL);
        pthread_cond_init(&trolleys_cond[i], NULL);
    }

    station_trolleyID = 0;
    signal(SIGUSR1, passenger_cleanup);
}

void start_coaster() {
    for (int i = 0; i < TOTAL_TROLLEYS; i++)
        pthread_create(&T_TROLLEYS[i], NULL, trolley_role, &TROLLEYS[i]);

    for (int i = 0; i < TOTAL_PASSENGERS; i++)
        pthread_create(&T_PASSENGERS[i], NULL, passenger_role, &PASSENGERS[i]);
}

void finalize_coaster() {

    for (int i = 0; i < TOTAL_TROLLEYS; i++)
        pthread_join(T_TROLLEYS[i], NULL);

    for (int i = 0; i < TOTAL_PASSENGERS; i++) {
        pthread_kill(T_PASSENGERS[i], SIGUSR1);
        pthread_join(T_PASSENGERS[i], NULL);
    }

    for (int i = 0; i < TOTAL_TROLLEYS; i++) {
        pthread_cond_destroy(&trolleys_cond[i]);
        pthread_mutex_destroy(&trolleys_mutex[i]);
        free(TROLLEYS[i].passengers);
    }

    pthread_cond_destroy(&trolley_empty_cond);
    pthread_cond_destroy(&trolley_ready_cond);

    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&passenger_mutex);
    pthread_mutex_destroy(&trolley_ready_mutex);
    pthread_mutex_destroy(&trolley_empty_mutex);

    free(trolleys_cond);
    free(trolleys_mutex);
    free(T_TROLLEYS);
    free(T_PASSENGERS);
    free(TROLLEYS);
    free(PASSENGERS);
}

void *passenger_role(void *initial_data) {
    Passenger *passenger = (Passenger *) initial_data;

    while (RUN) {
        pthread_mutex_lock(&passenger_mutex);

        passenger->trolleyID = station_trolleyID;
        TROLLEYS[station_trolleyID].passengers[TROLLEYS[station_trolleyID].passengersCount] = *passenger;
        TROLLEYS[station_trolleyID].passengersCount++;
        print_coaster_event(PASSENGER_IN, passenger->passengerID, TROLLEYS[station_trolleyID].passengersCount);

        if (TROLLEYS[station_trolleyID].passengersCount == TOTAL_CAPACITY) {
            pthread_cond_signal(&trolley_ready_cond);
            pthread_mutex_unlock(&trolley_ready_mutex);
        } else {
            pthread_mutex_unlock(&passenger_mutex);
        }

        pthread_mutex_lock(&trolleys_mutex[passenger->trolleyID]);
        TROLLEYS[station_trolleyID].passengersCount--;
        print_coaster_event(PASSENGER_OUT, passenger->passengerID, TROLLEYS[station_trolleyID].passengersCount);

        if (TROLLEYS[station_trolleyID].passengersCount == 0) {
            pthread_cond_signal(&trolley_empty_cond);
            pthread_mutex_unlock(&trolley_empty_mutex);
        }

        pthread_mutex_unlock(&trolleys_mutex[passenger->trolleyID]);
        passenger->trolleyID = -1;
    }

    print_coaster_event(PASSENGER_KILL, passenger->passengerID, -1);
    pthread_exit((void *) 0);
}

void passenger_cleanup(int sig) {
    if (sig == SIGUSR1) {
        print_coaster_event(PASSENGER_KILL, -1, -1);
        pthread_exit((void *) 0);
    }
}

void *trolley_role(void *initial_data) {
    Trolley *trolley = (Trolley *) initial_data;
    srand48(time(NULL));

    if (trolley->trolleyID == 0)
        pthread_mutex_lock(&passenger_mutex);

    for (int i = 0; i < trolley->loopsLeft; i++) {
        pthread_mutex_lock(&station_mutex);
        if (trolley->trolleyID != station_trolleyID)
            pthread_cond_wait(&trolleys_cond[trolley->trolleyID], &station_mutex);

        print_coaster_event(TROLLEY_OPEN, trolley->trolleyID, -1);

        if (i != 0) {
            pthread_mutex_unlock(&trolleys_mutex[trolley->trolleyID]);
            pthread_cond_wait(&trolley_empty_cond, &trolley_empty_mutex);
        }

        pthread_mutex_lock(&trolleys_mutex[trolley->trolleyID]);
        pthread_mutex_unlock(&passenger_mutex);
        pthread_cond_wait(&trolley_ready_cond, &trolley_ready_mutex);

        int startingPassengerID = trolley->passengers[lrand48() % TOTAL_CAPACITY].passengerID;
        print_coaster_event(PASSENGER_START, startingPassengerID, trolley->passengersCount);
        print_coaster_event(TROLLEY_CLOSE, trolley->trolleyID, -1);

        station_trolleyID++;
        station_trolleyID %= TOTAL_TROLLEYS;

        pthread_cond_signal(&trolleys_cond[station_trolleyID]);
        pthread_mutex_unlock(&station_mutex);

        print_coaster_event(TROLLEY_START, trolley->trolleyID, -1);

        usleep((int) drand48() * 1000000); // 0-1 sec

        print_coaster_event(TROLLEY_STOP, trolley->trolleyID, -1);
    }

    print_coaster_event(TROLLEY_KILL, trolley->trolleyID, 0);
    pthread_exit((void *) 0);
}