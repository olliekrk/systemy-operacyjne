#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef CW06_UTILS2
#define CW06_UTILS2

#define COMMAND_LENGTH 2048
#define MESSAGE_LENGTH 2048
#define FRIENDS_DELIMITER ","
#define NUMBER_OF_COMMANDS 10
#define MAX_FRIENDS 20
#define MAX_CLIENTS 20
#define SERVER_QUEUE_NAME "/SO_ServerQueue"
#define QUEUE_SIZE 30
#define MSIZE sizeof(struct q_message)

struct q_message {
    long mtype;
    pid_t client_id;
    char message[MESSAGE_LENGTH];
};

enum q_type {
    STOP = 10,
    INIT = 9,
    LIST = 8,
    FRIENDS = 7,
    ADD = 6,
    DEL = 5,
    ECHO = 4,
    _2ONE = 3,
    _2FRIENDS = 2,
    _2ALL = 1
};

void show_error(const char *message) {
    perror(message);
    exit(1);
}

int message_priority(enum q_type type) {
    switch (type) {
        case STOP:
            return 10;
        case INIT:
            return 9;
        case LIST:
        case FRIENDS:
            return 8;
        default:
            return 1;
    }
}

char *receive_client_queue_name() {
    char *queue_name = malloc(32 * sizeof(char));
    sprintf(queue_name, "/client_%d_%d", getpid(), rand() % 1000);
    return queue_name;
}

#endif //CW_06_UTILS2