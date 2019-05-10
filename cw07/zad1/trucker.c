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
int CONVEYOR_BELT_LOAD = 0;
int TRUCK_LOAD = 0;

// capacity related
int CONVEYOR_BELT_CAP = 0;

// IDs
int sem_id = -1;
int belt_id = -1;
conveyor_belt *belt = NULL;

// specific conveyor belt semaphores IDs
#define CAP_SEM 0
#define LOAD_SEM 1
#define SET_SEM 2

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

    trucker_loop();

    return 0;
}

void trucker_loop() {
    while (belt) {
        if (belt->current_cap > 0) {
            belt_item item = belt_pop(belt);
            if (item.weight + belt->current_load > belt->max_load){
                factory_event *full_event = generate_factory_event(belt, TRUCK_DEPARTURE);
                //todo: cd
            }
        }
    }
}

void trucker_cleanup() {
    /* todo fix:
     *  W przypadku, gdy trucker kończy pracę, powinien:
     *  1. zablokować semafor taśmy transportowej dla pracowników,
     *  2. załadować to co pozostało na taśmie
     */
    free(belt->items);
    belt->current_load = belt->current_cap = 0;
    if (shmdt(belt) == -1) fprintf(stderr, "Failed to detach belt from process memory\n");
    if (shmctl(belt_id, IPC_RMID, NULL) == -1) fprintf(stderr, "Failed to delete conveyor belt\n");
    if (semctl(sem_id, 0, IPC_RMID) == -1) fprintf(stderr, "Failed to delete semaphores set\n");
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
    factory_event *init_event = generate_factory_event(belt, TRUCK_ARRIVAL);
    print_factory_event(init_event);
}

void create_semaphores() {
    key_t sem_key = receive_sem_key();
    sem_id = semget(sem_key, 3, CREATION_FLAG);
    if (sem_id == -1) show_error("Failed to create semaphore(s)");

    set_semaphore(sem_id, CAP_SEM, CONVEYOR_BELT_CAP);
    set_semaphore(sem_id, LOAD_SEM, CONVEYOR_BELT_LOAD);
    set_semaphore(sem_id, SET_SEM, 1);
}

void print_factory_event(factory_event *event) {
    printf("NEW EVENT\n\t"
           "TIME: %ld s\t%ld ms\n\t"
           "PID: %d\n\t"
           "CURRENT LOAD: %d kg\tCURRENT CAP: %d items\n\t",
           event->time.tv_sec,
           event->time.tv_usec,
           event->pid,
           event->current_cb_load,
           event->current_cb_cap);
}

void interrupt_handler(int s) {
    printf("(!)\tTrucker interrupted by SIGINT\n");
    exit(3);
}