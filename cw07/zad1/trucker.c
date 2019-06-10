#include "trucker.h"

int main(int argc, char **argv) {
    if (argc < 4)
        show_error("Invalid number of arguments:\n\t<TRUCK_LOAD> <BELT_LOAD> <BELT_CAP>");

    TRUCK_LOAD = strtol(argv[1], NULL, 10);
    CONVEYOR_BELT_LOAD = strtol(argv[2], NULL, 10);
    CONVEYOR_BELT_CAP = strtol(argv[3], NULL, 10);
    if (CONVEYOR_BELT_CAP > MAXIMUM_BELT_CAP)
        show_error("Conveyor belt maximum capacity is exceeded");

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
    if (locking) semaphore_lock_take(sem_id); // lock

    if (item.weight + CURRENT_TRUCK_LOAD > TRUCK_LOAD) trucker_unload_truck();
    else trucker_load_truck();

    if (locking) semaphore_lock_release(sem_id);// unlock
    return 1;
}

void trucker_unload_truck() {
    print_event(generate_event(belt, TRUCK_DEPARTURE));
    sleep(1);
    CURRENT_TRUCK_LOAD = 0;
    print_event(generate_event(belt, TRUCK_ARRIVAL));
}

void trucker_load_truck() {
    // shared memory operations
    belt_item item = belt_peek(belt);
    belt_pop(belt);

    //semaphores operations
    semaphore_item_to_truck(sem_id, item.weight);

    CURRENT_TRUCK_LOAD += item.weight;

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
           CURRENT_TRUCK_LOAD, diff.tv_sec, diff.tv_usec);
}

void trucker_cleanup() {
    //1.
    semaphore_set(sem_id, GLOBAL_LOCK_SEM, 0);

    //2.
    while (trucker_loop(0) == 1) sleep(1);

    if (semctl(sem_id, 0, IPC_RMID) == -1) fprintf(stderr, "Failed to delete semaphores set\n");
    if (shmdt(belt) == -1) fprintf(stderr, "Failed to detach belt from process memory\n");
    if (shmctl(belt_id, IPC_RMID, NULL) == -1) fprintf(stderr, "Failed to delete conveyor belt\n");

    printf("Trucker has finished its work.\n");
    exit(0);
}

void create_conveyor_belt() {
    belt_id = shmget(receive_belt_key(), sizeof(conveyor_belt), CREATION_FLAG);
    if (belt_id == -1) show_error("Failed to create conveyor belt");

    belt = shmat(belt_id, NULL, 0);
    if (belt == (void *) -1) show_error("Failed to attach belt to process memory");

    belt->current_cap = 0;
    belt->current_load = 0;

    // initialize with event of truck arrival
    print_event(generate_event(belt, TRUCK_ARRIVAL));
}

void create_semaphores() {
    key_t sem_key = receive_sem_key();
    sem_id = semget(sem_key, NUMBER_OF_SEMAPHORES, CREATION_FLAG);
    if (sem_id == -1) show_error("Failed to create semaphore(s)");

    semaphore_set(sem_id, BELT_CAP_SEM, CONVEYOR_BELT_CAP);
    semaphore_set(sem_id, BELT_LOAD_SEM, CONVEYOR_BELT_LOAD);
    semaphore_set(sem_id, GLOBAL_LOCK_SEM, 1);
    semaphore_set(sem_id, LOADERS_SEM, 1);
}

void interrupt_handler(int s) {
    printf("(!)\tTrucker interrupted by SIGINT\n");
    exit(3);
}