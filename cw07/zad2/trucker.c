#include "trucker.h"

int main(int argc, char **argv) {
    if (argc < 4)
        show_error("Invalid number of arguments:\n\t<TRUCK_LOAD> <BELT_LOAD> <BELT_CAP>");

    TRUCK_LOAD = strtol(argv[1], NULL, 10);
    CONVEYOR_BELT_LOAD = strtol(argv[2], NULL, 10);
    CONVEYOR_BELT_CAP = strtol(argv[3], NULL, 10);
    if (CONVEYOR_BELT_CAP > MAXIMUM_BELT_CAP)
        show_error("Conveyor belt maximum capacity cannot be exceeded");

    atexit(trucker_cleanup);
    signal(SIGINT, interrupt_handler);
    create_semaphores();
    create_conveyor_belt();

    while (belt) {
        trucker_loop(1);
        sleep(1); // comment to let trucker work non-stop
    }

    exit(3);
}

int trucker_loop(int locking) {
    if (belt->current_cap == 0) {
        print_event(generate_event(belt, TRUCK_WAIT));
        return 0;
    }

    belt_item item = belt_peek(belt);
    if (locking) sem_wait(belt_lock_sem); // lock

    if (item.weight + current_truck_load > TRUCK_LOAD) trucker_unload_truck();
    else trucker_load_truck();

    if (locking) sem_post(belt_lock_sem); // unlock
    return 1;
}

void trucker_unload_truck() {
    print_event(generate_event(belt, TRUCK_DEPARTURE));
    sleep(1);
    current_truck_load = 0;
    print_event(generate_event(belt, TRUCK_ARRIVAL));
}

void trucker_load_truck() {
    // shared memory operations
    belt_item item = belt_peek(belt);
    belt_pop(belt);

    // semaphores operations
    sem_post(belt_cap_sem);
    for (int i = 0; i < item.weight; i++) sem_post(belt_load_sem);

    current_truck_load += item.weight;

    belt_event event;
    event.type = TRUCK_LOADING;
    event.pid = item.loader_pid;
    event.current_cb_cap = belt->current_cap;
    event.current_cb_load = belt->current_load;
    get_current_time(&event.time);
    print_event(&event);

    tv diff;
    timersub(&event.time, &item.time, &diff);
    printf("\tCURRENT TRUCK LOAD: %d\n"
           "\tTIME ON BELT: %ld s\t%ld us\n",
           current_truck_load, diff.tv_sec, diff.tv_usec);
}

void trucker_cleanup() {
    //1.
    sem_wait(belt_lock_sem);

    //2.
    while (trucker_loop(0) == 1) sleep(1);

    sem_close(loaders_sem);
    sem_close(belt_lock_sem);
    sem_close(belt_cap_sem);
    sem_close(belt_load_sem);
    sem_unlink(LOADERS_SEM_NAME);
    sem_unlink(BELT_LOCK_SEM_NAME);
    sem_unlink(BELT_CAP_SEM_NAME);
    sem_unlink(BELT_LOAD_SEM_NAME);

    munmap(belt, sizeof(conveyor_belt));
    shm_unlink(BELT_NAME);

    printf("Trucker has finished its work.\n");
    exit(0);
}

void create_conveyor_belt() {
    belt_id = shm_open(BELT_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (belt_id == -1) show_error("Failed to create conveyor belt");
    if (ftruncate(belt_id, sizeof(conveyor_belt)) == -1) show_error("Failed to allocate shared memory segment");
    belt = mmap(NULL, sizeof(conveyor_belt), PROT_READ | PROT_WRITE, MAP_SHARED, belt_id, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");

    belt->current_cap = 0;
    belt->current_load = 0;

    // initialize with event of truck arrival
    print_event(generate_event(belt, TRUCK_ARRIVAL));
}

void create_semaphores() {
    belt_cap_sem = sem_open(BELT_CAP_SEM_NAME, O_CREAT, 0666, CONVEYOR_BELT_CAP);
    belt_load_sem = sem_open(BELT_LOAD_SEM_NAME, O_CREAT, 0666, CONVEYOR_BELT_LOAD);
    belt_lock_sem = sem_open(BELT_LOCK_SEM_NAME, O_CREAT, 0666, 1);
    loaders_sem = sem_open(LOADERS_SEM_NAME, O_CREAT, 0666, 1);

    if (!belt_cap_sem || !belt_load_sem || !belt_lock_sem || !loaders_sem)
        show_error("Failed to create semaphore(s)");
}

void interrupt_handler(int s) {
    printf("(!)\tTrucker interrupted by SIGINT\n");
    exit(3);
}