#include "loader.h"

int main(int argc, char **argv) {
    if (argc < 2)
        show_error("Invalid number of arguments:\n\t<ITEM_WEIGHT> [<NUMBER_OF_CYCLES>]");

    ITEM_WEIGHT = strtol(argv[1], NULL, 10);
    if (argc > 2) NUMBER_OF_CYCLES = strtol(argv[2], NULL, 10);

    atexit(loader_cleanup);
    signal(SIGINT, interrupt_handler);
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
    int belt_cap = 0;
    int belt_load = 0;

    while (abs(NUMBER_OF_CYCLES) > 0) {
        if (sem_trywait(belt_lock_sem) == 0) { // lock
            sem_getvalue(belt_cap_sem, &belt_cap);
            sem_getvalue(belt_load_sem, &belt_load);
            printf("seg fault1\n");
            if (belt_cap > 0 && belt_load >= ITEM_WEIGHT) {
                sem_wait(belt_cap_sem);
                for (int i = 0; i < ITEM_WEIGHT; i++) sem_wait(belt_load_sem);
                loader_load_belt(item);
                was_waiting = 0;
                sleep(1);
            }
            printf("seg fault2\n");
            sem_post(belt_lock_sem); // unlock
        } else {
            if (sem_getvalue(belt_lock_sem, NULL) == -1) exit(3);
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
    sem_close(belt_cap_sem);
    sem_close(belt_load_sem);
    sem_close(belt_lock_sem);
    munmap(belt, sizeof(conveyor_belt));
    printf("Loader has finished its work.\n");
    exit(0);
}

void access_conveyor_belt() {
    belt_id = shm_open(BELT_NAME, O_RDWR, 0666);
    if (belt_id == -1) show_error("Failed to access conveyor belt");
    if (ftruncate(belt_id, sizeof(conveyor_belt)) == -1) show_error("Failed to allocate shared memory segment");
    belt = mmap(NULL, sizeof(conveyor_belt), PROT_READ | PROT_WRITE, MAP_SHARED, belt_id, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");
}

void access_semaphores() {
    belt_cap_sem = sem_open(BELT_CAP_SEM_NAME, 0);
    belt_load_sem = sem_open(BELT_LOAD_SEM_NAME, 0);
    belt_lock_sem = sem_open(BELT_LOCK_SEM_NAME, 0);
    if (!belt_cap_sem || !belt_load_sem || !belt_lock_sem)
        show_error("Failed to create semaphore(s)");
}

void interrupt_handler(int s) {
    printf("(!)\tLoader interrupted by SIGINT\n");
    exit(3);
}