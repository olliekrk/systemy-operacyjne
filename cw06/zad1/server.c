//
// Created by olliekrk on 03.05.19.
//


#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "utils6.h"

struct q_client {
    pid_t pid;
    int queue_id;
    int friends[MAX_FRIENDS];
    int no_friends;
};

int running = true;
int server_queue_id = -1;
struct q_client clients[MAX_CLIENTS];

void SIGINT_handler(int sig);

void process_message(struct q_message *msg);

// general functions

bool is_client_available(int client_id) {
    return client_id < MAX_CLIENTS && client_id >= 0 && clients[client_id].queue_id != -1;
}

void close_server() {
    if (msgctl(server_queue_id, IPC_RMID, NULL) == -1) show_error("Error while deleting queue");
    else exit(0);
}

void initialize_server() {
    atexit(close_server);
    signal(SIGINT, SIGINT_handler);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].queue_id = -1;
        clients[i].no_friends = 0;
    }

    server_queue_id = msgget(receive_server_queue_key(), IPC_CREAT | 0666);
    if (server_queue_id == -1) show_error("Error while creating server queue");
    printf("Accessed server's queue with key: %d\n", server_queue_id);
}

// messaging

int send_message(int client_id, enum q_type type, char message[MESSAGE_LENGTH]) {
    struct q_message response;
    response.mtype = type;
    response.client_id = client_id;
    strcpy(response.message, message);

    if (!is_client_available(client_id)) {
        fprintf(stderr, "Client of such ID is unavailable. Response has not been sent\n");
        return -1;
    }

    if (msgsnd(clients[client_id].queue_id, &response, MSIZE, IPC_NOWAIT) == -1) {
        fprintf(stderr, "Error while sending response message to the client\n");
        return -1;
    }

    return 0;
}

void add_friends(pid_t client_id, char friends_list[MESSAGE_LENGTH]) {
    char *friend = strtok(friends_list, (const char *) FRIENDS_DELIMITER);

    while (friend != NULL && clients[client_id].no_friends < MAX_FRIENDS) {
        int friend_id = (int) strtol(friend, NULL, 10);

        // check if such friend is already on the list
        for (int i = 0; i < clients[client_id].no_friends; i++) {
            if (friend_id == clients[client_id].friends[i])
                friend_id = -1;
        }

        // if it's not on the list and its ID is valid then add to friends
        if (friend_id < MAX_FRIENDS && friend_id >= 0 && friend_id != client_id) {
            clients[client_id].friends[clients[client_id].no_friends] = friend_id;
            clients[client_id].no_friends++;
        }

        friend = strtok(NULL, (const char *) FRIENDS_DELIMITER);
    }
}

// chat commands 'handlers'
void execute_stop(struct q_message *msg) {

    //shutting down only one client
    if (msg != NULL) {
        printf("Executing STOP for client: %d\n", msg->client_id);
        clients[msg->client_id].queue_id = -1;
        return;
    }

    //else shutting down whole server and all clients
    printf("Executing STOP globally\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id != -1) {
            send_message(i, STOP, "");
            kill(clients[i].pid, SIGUSR1);
        }
    }

    running = false;
}

void execute_list(struct q_message *msg) {
    printf("Executing LIST for client: %d\n", msg->client_id);
    char buffer[MESSAGE_LENGTH];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id >= 0) {
            sprintf(buffer, "Client's ID: %d\tClient Queue's ID:%d\n", i, clients[i].queue_id);
        }
    }
    send_message(msg->client_id, LIST, buffer);
}

void execute_init(struct q_message *msg) {
    // find the lowest available id
    int available_id;
    for (available_id = 0; available_id < MAX_CLIENTS; available_id++)
        if (clients[available_id].queue_id == -1)
            break;

    if (available_id >= MAX_CLIENTS) {
        fprintf(stdout, "INIT failed: Clients limit is reached");
        return;
    }

    int client_queue_id;
    sscanf(msg->message, "%d", &client_queue_id);
    clients[available_id].queue_id = client_queue_id;
    clients[available_id].pid = msg->client_id;
    clients[available_id].no_friends = 0;

    char initialize_message[MESSAGE_LENGTH];
    sprintf(initialize_message, "%d", available_id);
    send_message(available_id, INIT, initialize_message);
    printf("Executed INIT for new client %d with PID: %d\n", available_id, msg->client_id);
}

void execute_echo(struct q_message *msg) {
    printf("Executing ECHO for client: %d\n", msg->client_id);
    char response_message[MESSAGE_LENGTH];
    char datetime[31];

    FILE *date = popen("date", "r");
    fread(datetime, sizeof(char), 31, date);
    pclose(date);

    sprintf(response_message, "%s %s", msg->message, datetime);
    send_message(msg->client_id, ECHO, response_message);
}

void execute_friends(struct q_message *msg) {
    printf("Executing FRIENDS for client: %d\n", msg->client_id);
    if (!is_client_available(msg->client_id)) {
        fprintf(stderr, "Invalid client ID: %d\n", msg->client_id);
        return;
    }

    char friends_list[MESSAGE_LENGTH];
    int n = sscanf(msg->message, "%s", friends_list);
    clients[msg->client_id].no_friends = 0;

    if (n == 1)
        add_friends(msg->client_id, friends_list);
}

void execute_add(struct q_message *msg) {
    printf("Executing ADD for client: %d\n", msg->client_id);
    if (!is_client_available(msg->client_id)) {
        fprintf(stderr, "Invalid client ID: %d\n", msg->client_id);
        return;
    }

    char friends_list[MESSAGE_LENGTH];
    int n = sscanf(msg->message, "%s", friends_list);

    if (n == EOF || n == 0)
        fprintf(stderr, "Error while adding friends of client %d\n", msg->client_id);
    else if (n == 1) {
        add_friends(msg->client_id, friends_list);
    }
}

void execute_del(struct q_message *msg) {
    printf("Executing DEL for client: %d\n", msg->client_id);
    if (!is_client_available(msg->client_id)) {
        fprintf(stderr, "Invalid client ID: %d\n", msg->client_id);
        return;
    }

    char friends_list[MESSAGE_LENGTH];
    int n = sscanf(msg->message, "%s", friends_list);

    if (n == EOF || n == 0) {
        fprintf(stderr, "Error while deleting friends of client %d\n", msg->client_id);
        return;
    }

    // used for accessing single friends in given list-arg
    char *friend;
    int *no_friends = &clients[msg->client_id].no_friends;
    int friend_id;

    if (n == 1) {
        friend = strtok(friends_list, (const char *) FRIENDS_DELIMITER);
        while (friend != NULL && (*no_friends) > 0) {
            friend_id = strtol(friend, NULL, 10);

            int i;
            for (i = 0; i < (*no_friends); i++) {
                if (friend_id == clients[msg->client_id].friends[i])
                    break;
            }

            // if no friend with such ID id was found do nothing
            if (i >= (*no_friends))
                friend_id = -1;

            // otherwise delete that friend
            if (friend_id >= 0 && friend_id < MAX_CLIENTS && friend_id != msg->client_id) {
                clients[msg->client_id].friends[i] = clients[msg->client_id].friends[(*no_friends) - 1];
                (*no_friends)--;
            }

            friend = strtok(NULL, (const char *) FRIENDS_DELIMITER);
        }
    }
}

void prepare_date_response(pid_t client_id, char *message, char *response) {
    char datetime[31];

    FILE *date = popen("date", "r");
    fread(datetime, sizeof(char), 31, date);
    pclose(date);

    sprintf(response, "%s\t%d\t%s\n", message, client_id, datetime);
}

void execute_2all(struct q_message *msg) {
    char response[MESSAGE_LENGTH];
    prepare_date_response(msg->client_id, msg->message, response);

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (i != msg->client_id && clients[i].queue_id != -1) {
            send_message(i, _2ALL, response);
            kill(clients[i].pid, SIGUSR1);
        }
}

void execute_2friends(struct q_message *msg) {
    char response[MESSAGE_LENGTH];
    prepare_date_response(msg->client_id, msg->message, response);

    for (int i = 0; i < clients[msg->client_id].no_friends; i++) {
        int receiver = clients[msg->client_id].friends[i];
        if (is_client_available(receiver)) {
            send_message(receiver, _2FRIENDS, response);
            kill(clients[receiver].pid, SIGUSR1);
        }
    }
}

void execute_2one(struct q_message *msg) {
    int receiver;
    char text[MESSAGE_LENGTH];
    sscanf(msg->message, "%d %s", &receiver, text);

    char response[MESSAGE_LENGTH];
    prepare_date_response(receiver, text, response);

    if (is_client_available(receiver)) {
        send_message(receiver, _2ONE, response);
        kill(clients[receiver].pid, SIGUSR1);
    }
}

// signal handlers
void SIGINT_handler(int sig) {
    execute_stop(NULL);
}

int main() {
    initialize_server();
    struct q_message buffer;
    while (running) {
        if (msgrcv(server_queue_id, &buffer, MSIZE, -(NUMBER_OF_COMMANDS), 0) != -1) {
            process_message(&buffer);
        }
    }

    exit(0);
}

// server loop
void process_message(struct q_message *msg) {
    switch (msg->mtype) {
        case STOP:
            execute_stop(msg);
            break;
        case ECHO:
            execute_echo(msg);
            break;
        case INIT:
            execute_init(msg);
            break;
        case LIST:
            execute_list(msg);
            break;
        case FRIENDS:
            execute_friends(msg);
            break;
        case _2ALL:
            execute_2all(msg);
            break;
        case _2FRIENDS:
            execute_2friends(msg);
            break;
        case _2ONE:
            execute_2one(msg);
            break;
        case ADD:
            execute_add(msg);
            break;
        case DEL:
            execute_del(msg);
            break;
    }
}