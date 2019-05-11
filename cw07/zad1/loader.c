#include "loader.h"

int main(int argc, char **argv) {
    if (argc < 2)
        show_error("Invalid number of arguments:\n\t<ITEM_WEIGHT> [<NUMBER_OF_LOADERS>] [<NUMBER_OF_CYCLES>]");

    ITEM_WEIGHT = strtol(argv[1], NULL, 10);
    if (argc > 2) NUMBER_OF_LOADERS = strtol(argv[2], NULL, 10);
    if (argc > 3) NUMBER_OF_CYCLES = strtol(argv[3], NULL, 10);

    atexit(loader_cleanup);
    access_semaphores();
    access_conveyor_belt();

    loader_loop();
    exit(3);
}

void loader_loop() {
    printf("Loader with PID %d started working.\n", getpid());
    belt_item item;
    item.weight = ITEM_WEIGHT;
    item.loader_pid = getpid();

    int was_waiting = 0; // to reduce frequency of printing 'WAIT' events

    while (abs(NUMBER_OF_CYCLES) > 0) {
        if (semaphore_item_to_belt(sem_id, ITEM_WEIGHT) == 0) { // lock
            loader_load_belt(item);
            semaphore_lock_release(sem_id); // unlock
            was_waiting = 0;
            sleep(1);
        } else {
            if (semaphore_get(sem_id, GLOBAL_LOCK_SEM) == -1) exit(3);
            if (was_waiting == 0) print_event(generate_event(belt, LOADER_WAIT));
            was_waiting = 1;
        }
    }
}

void loader_load_belt(belt_item item) {
    get_current_time(&item.time);
    belt_push(belt, item);
    print_event(generate_event(belt, ITEM_LOADED));
    NUMBER_OF_CYCLES--;
}

void loader_cleanup() {
    shmdt(belt);
    printf("Loader has finished its work.\n");
    exit(0);
}

void access_conveyor_belt() {
    key_t belt_key = receive_belt_key();
    belt_id = shmget(belt_key, sizeof(conveyor_belt), 0);
    if (belt_id == -1) show_error("Failed to access conveyor belt");

    belt = shmat(belt_id, NULL, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");
}

void access_semaphores() {
    key_t sem_key = receive_sem_key();
    sem_id = semget(sem_key, NUMBER_OF_SEMAPHORES, 0);
    if (sem_id == -1) show_error("Failed to access semaphore(s)");
}

void interrupt_handler(int s) {
    printf("(!)\tLoader interrupted by SIGINT\n");
    exit(3);
}