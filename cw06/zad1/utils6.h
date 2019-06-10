//
// Created by olliekrk on 03.05.19.
//

#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef CW06_UTILS
#define CW06_UTILS

#define COMMAND_LENGTH 2048
#define MESSAGE_LENGTH 2048
#define FRIENDS_DELIMITER ","
#define NUMBER_OF_COMMANDS 10
#define MAX_FRIENDS 20
#define MAX_CLIENTS 20
#define SEED 96
#define MSIZE sizeof(struct q_message) - sizeof(long)

struct q_message {
    long mtype;
    pid_t client_id;
    char message[MESSAGE_LENGTH];
};

enum q_type {
    STOP = 1,
    INIT = 2,
    LIST = 3,
    FRIENDS = 4,
    ADD = 5,
    DEL = 6,
    ECHO = 7,
    _2ONE = 8,
    _2FRIENDS = 9,
    _2ALL = 10
};

void show_error(const char *message) {
    perror(message);
    exit(1);
}

// for generating queue keys
key_t receive_key(int id) {
    char *home_path = getenv("HOME");
    if (!home_path)
        show_error("$HOME is unavailable");
    key_t generated_key = ftok(home_path, id);
    if (generated_key == -1)
        show_error("Unable to generate key.");
    return generated_key;
}

key_t receive_server_queue_key() {
    return receive_key(SEED);
}

key_t receive_client_queue_key() {
    return receive_key(getpid());
}

#endif //CW_06_UTILS