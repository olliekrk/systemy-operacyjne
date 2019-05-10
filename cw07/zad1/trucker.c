/*
responsibilities:
- managing common memory (conveyor belt CB unloading)
- SIGINT handler for cleanup
- has to be run before loader.c (handle this case in loader.c)
- before closing it should:
    block semaphore,
    clean up CB load
    + in POSIX notify workers to perform their cleanup
*/

#include "trucker.h"

#define CREATION_FLAG IPC_CREAT | 0666 /*| IPC_EXCL*/

// weight related
int TRUCK_LOAD = 0;
int CONVEYOR_BELT_LOAD = 0;

// capacity related
int CONVEYOR_BELT_CAP = 0;

// IDs
int sem_id = -1;
int belt_id = -1;
conveyor_belt *belt = NULL;

// specific conveyor belt semaphores IDs
#define BELT_CAP_SEM 0
#define BELT_LOAD_SEM 1
#define GLOBAL_LOCK_SEM 2

int main(int argc, char **argv) {
    if (argc < 4)
        show_error("Invalid number of arguments");

    TRUCK_LOAD = strtol(argv[1], NULL, 10);
    CONVEYOR_BELT_LOAD = strtol(argv[2], NULL, 10);
    CONVEYOR_BELT_CAP = strtol(argv[3], NULL, 10);

    atexit(trucker_cleanup);
    signal(SIGINT, interrupt_handler);

    create_semaphores();
    create_conveyor_belt();

    while (belt) {
        trucker_loop();
        sleep(1);
    }

    exit(3);
}

int trucker_loop() {
    if (belt->current_cap > 0) {
        belt_item item = belt_peek(belt);
        if (item.weight + belt->current_load > TRUCK_LOAD) {
            print_event(generate_event(belt, TRUCK_DEPARTURE));
            sleep(1);
            print_event(generate_event(belt, TRUCK_ARRIVAL));
            TRUCK_LOAD = 0;
        } else {
            belt_pop(belt);
            semaphore_load_truck(sem_id, item.weight);

            belt_event event;
            event.type = TRUCK_LOADING;
            event.pid = item.loader_pid;
            event.current_cb_cap = belt->current_cap;
            event.current_cb_load = belt->current_load;
            get_current_time(&item.time);

            print_event(&event);

            tv diff;
            timersub(&item.time, &event.time, &diff);
            printf("\tTIME ON BELT: %ld s\t%ld ms\n", diff.tv_sec, diff.tv_usec);
        }
        return 1;
    } else {
        print_event(generate_event(belt, TRUCK_WAIT));
        return 0;
    }
}

void trucker_cleanup() {
    /*
     *  W przypadku, gdy trucker kończy pracę, powinien:
     *  1. zablokować semafor taśmy transportowej dla pracowników,
     *  2. załadować to co pozostało na taśmie
     */

    //1.
    semaphore_set(sem_id, GLOBAL_LOCK_SEM, 0);

    //2.
    while (trucker_loop() == 1) {}

    free(belt->items);
    belt->current_load = belt->current_cap = 0;
    if (shmdt(belt) == -1) fprintf(stderr, "Failed to detach belt from process memory\n");
    if (shmctl(belt_id, IPC_RMID, NULL) == -1) fprintf(stderr, "Failed to delete conveyor belt\n");
    if (semctl(sem_id, 0, IPC_RMID) == -1) fprintf(stderr, "Failed to delete semaphores set\n");

    exit(0);
}

void create_conveyor_belt() {
    key_t belt_key = receive_belt_key();

    belt_id = shmget(belt_key, sizeof(conveyor_belt), CREATION_FLAG);
    if (belt_id == -1) show_error("Failed to create conveyor belt");

    belt = shmat(belt_id, NULL, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");

    belt->current_cap = 0;
    belt->max_cap = CONVEYOR_BELT_CAP;
    belt->current_load = 0;
    belt->max_load = CONVEYOR_BELT_LOAD;
    belt->items = calloc(CONVEYOR_BELT_CAP, sizeof(belt_item));

    // initialize with event of truck arrival
    belt_event *init_event = generate_event(belt, TRUCK_ARRIVAL);
    print_event(init_event);
}

void create_semaphores() {
    key_t sem_key = receive_sem_key();
    sem_id = semget(sem_key, NUMBER_OF_SEMAPHORES, CREATION_FLAG);
    if (sem_id == -1) show_error("Failed to create semaphore(s)");

    semaphore_set(sem_id, BELT_CAP_SEM, CONVEYOR_BELT_CAP);
    semaphore_set(sem_id, BELT_LOAD_SEM, CONVEYOR_BELT_LOAD);
    semaphore_set(sem_id, GLOBAL_LOCK_SEM, 1);
}

void interrupt_handler(int s) {
    printf("(!)\tTrucker interrupted by SIGINT\n");
    exit(3);
}