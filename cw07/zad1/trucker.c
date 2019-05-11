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
        sleep(1);
    }

    exit(3);
}

int trucker_loop(int locking) {
    if (belt->current_cap > 0) {
        belt_item item = belt_peek(belt);
        if (item.weight + current_truck_load > TRUCK_LOAD) {
            trucker_unload_truck(locking);
            return 1;
        } else {
            if (locking) semaphore_item_to_truck(sem_id, item.weight);// lock

            trucker_load_truck();

            if (locking) semaphore_lock_release(sem_id);// unlock
            return 1;
        }
    } else {
        print_event(generate_event(belt, TRUCK_WAIT));
        return 0;
    }
}

void trucker_unload_truck(int locking) {
    // according to the instructions, new packages must not land on the belt if the truck is away
    if (locking) semaphore_lock_take(sem_id);

    print_event(generate_event(belt, TRUCK_DEPARTURE));
    sleep(1);
    print_event(generate_event(belt, TRUCK_ARRIVAL));
    current_truck_load = 0;

    if (locking) semaphore_lock_release(sem_id);
}

void trucker_load_truck() {
    belt_item item = belt_peek(belt);
    belt_pop(belt);

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
    printf("\tCURRENT TRUCK LOAD: %d\n\tTIME ON BELT: %ld s\t%ld ms\n", current_truck_load, diff.tv_sec, diff.tv_usec);
}

void trucker_cleanup() {
    //1.
    semaphore_set(sem_id, GLOBAL_LOCK_SEM, 0);

    //2.
    while (trucker_loop(0) == 1)
        sleep(1);

    if (semctl(sem_id, 0, IPC_RMID) == -1) fprintf(stderr, "Failed to delete semaphores set\n");
    if (shmdt(belt) == -1) fprintf(stderr, "Failed to detach belt from process memory\n");
    if (shmctl(belt_id, IPC_RMID, NULL) == -1) fprintf(stderr, "Failed to delete conveyor belt\n");

    printf("Trucker has finished its work.\n");
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