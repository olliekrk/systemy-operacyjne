#include "loader.h"

/*
 * it has to run after trucker.c has started
 * */

int ITEM_WEIGHT = 0;
int NUMBER_OF_CYCLES = -1; // infinity by default
int NUMBER_OF_LOADERS = 1; // one by default

int sem_id = -1;
int belt_id = -1;
conveyor_belt *belt = NULL;

int main(int argc, char **argv) {
    if (argc < 2)
        show_error("Invalid number of arguments");

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
    belt_item item;
    item.weight = ITEM_WEIGHT;
    item.loader_pid = getpid();
    while (abs(NUMBER_OF_CYCLES) > 0) {
        // todo
        get_current_time(&item.time);

        semaphore_load_item(sem_id, item.weight);
        semaphore_lock_take(sem_id);

        belt_push(belt, item);
        print_event(generate_event(belt, ITEM_LOADED));

        semaphore_lock_release(sem_id);

        NUMBER_OF_CYCLES--;
    }
}

void loader_cleanup() {
    shmdt(belt);
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