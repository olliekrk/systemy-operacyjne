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

    int belt_cap = 0;
    int belt_load = 0;
    int loaders_lock = 0;
    int item_placed = 0;

    tv wait_start, wait_end, wait_time;

    while (abs(NUMBER_OF_CYCLES) > 0) {
        if (sem_getvalue(loaders_sem, &loaders_lock) == -1) exit(3);

        print_event(generate_event(belt, LOADER_WAIT));
        get_current_time(&wait_start);

        sem_wait(loaders_sem); // lock the privilege to put the item

        for (item_placed = 0; item_placed == 0;) {
            sem_wait(belt_lock_sem); // lock the belt

            sem_getvalue(belt_cap_sem, &belt_cap);
            sem_getvalue(belt_load_sem, &belt_load);
            if (belt_cap > 0 && belt_load >= ITEM_WEIGHT) {
                loader_load_belt(item);
                item_placed = 1;

                get_current_time(&wait_end);
                timersub(&wait_end, &wait_start, &wait_time);
                printf("\tTIME WAITING: %ld s\t%ld us\n", wait_time.tv_sec, wait_time.tv_usec);
            }

            sem_post(belt_lock_sem); // unlock the belt
        }

        sem_post(loaders_sem); // unlock the privilege to put the item
        sleep(1); // comment to let loader work non-stop
    }
}

void loader_load_belt(belt_item item) {
    // shared memory operations
    get_current_time(&item.time);
    belt_push(belt, item);

    // semaphores operations
    sem_wait(belt_cap_sem);
    for (int i = 0; i < ITEM_WEIGHT; i++) sem_wait(belt_load_sem);

    print_event(generate_event(belt, ITEM_LOADED));
    if (NUMBER_OF_CYCLES > 0) NUMBER_OF_CYCLES--;
}

void loader_cleanup() {
    sem_close(belt_cap_sem);
    sem_close(belt_load_sem);
    sem_close(belt_lock_sem);
    sem_close(loaders_sem);
    munmap(belt, sizeof(conveyor_belt));
    printf("Loader has finished its work.\n");
    exit(0);
}

void access_conveyor_belt() {
    belt_id = shm_open(BELT_NAME, O_RDWR, 0666);
    if (belt_id == -1) show_error("Failed to access conveyor belt");
    if (ftruncate(belt_id, sizeof(conveyor_belt)) == -1) show_error("Failed to access shared memory segment");
    belt = mmap(NULL, sizeof(conveyor_belt), PROT_READ | PROT_WRITE, MAP_SHARED, belt_id, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");
}

void access_semaphores() {
    belt_cap_sem = sem_open(BELT_CAP_SEM_NAME, 0);
    belt_load_sem = sem_open(BELT_LOAD_SEM_NAME, 0);
    belt_lock_sem = sem_open(BELT_LOCK_SEM_NAME, 0);
    loaders_sem = sem_open(LOADERS_SEM_NAME, 0);

    if (!belt_cap_sem || !belt_load_sem || !belt_lock_sem || !loaders_sem)
        show_error("Failed to access semaphore(s)");
}

void interrupt_handler(int s) {
    printf("(!)\tLoader interrupted by SIGINT\n");
    exit(3);
}