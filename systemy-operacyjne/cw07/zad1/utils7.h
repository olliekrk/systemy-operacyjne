#ifndef CW7_UTILS
#define CW7_UTILS

#include <sys/shm.h>
#include <sys/ipc.h>
#include <time.h>

// weight related
#define MAX_ITEM_WEIGHT 10

typedef factory_event{
    enum event_type type;
    struct timeval time;
    
    pid_t pid; // PID for worker events
    int current_cb_cap; // cb = conveyor belt
    int current_cb_load; 

} factory_event;

enum event_type {
    TRUCK_ARRIVAL,
    TRUCK_LOADING,
    TRUCK_DEPARTURE,
    TRUCK_WAIT,
    LOADER_WAIT,
    ITEM_LOADED
};

void show_error(const char *message) {
    perror(message);
    exit(1);
}

#endif //CW7_UTILS