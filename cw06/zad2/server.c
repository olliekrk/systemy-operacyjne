#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <mqueue.h>
#include <sys/ipc.h>
#include <time.h>

#include "utils6.h"

struct q_client {
    mqd_t queue_id;
    int friends[MAX_FRIENDS];
    int no_friends;
    pid_t pid;
};

bool running = true;
mqd_t server_queue_id = -1;
struct q_client clients[MAX_CLIENTS];

void SIGINT_handler(int sig);

// general functions & utils
bool is_client_available(int client_id) {
    return client_id < MAX_CLIENTS && client_id >= 0 && clients[client_id].queue_id != -1;
}

void close_server() {
    if (mq_close(server_queue_id) == -1 || mq_unlink(SERVER_QUEUE_NAME) == -1)
        show_error("Error while closing the server");
    exit(0);
}

void initialize_server() {
    atexit(close_server);
    signal(SIGINT, SIGINT_handler);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].queue_id = -1;
        clients[i].no_friends = 0;
    }

    struct mq_attr queue_attr;
    queue_attr.mq_maxmsg = QUEUE_SIZE;
    queue_attr.mq_msgsize = MSIZE;

    server_queue_id = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &queue_attr);
    if (server_queue_id == -1)show_error("Error while creating server queue");
    printf("Accessed server's queue with key: %d\n", server_queue_id);
}

void prepare_date_response(pid_t client_id, char *message, char *response) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(response, "%s\t%d\t%s\n", message, client_id, asctime(timeinfo));
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

    if (mq_send(clients[client_id].queue_id, (char *) &response, MSIZE, message_priority(type)) == -1) {
        fprintf(stderr, "Error while sending response message");
        return -1;
    }

    return 0;
}

void add_friends(pid_t client_id, char friends_list[MESSAGE_LENGTH]) {
    char *friend = strtok(friends_list, FRIENDS_DELIMITER);
    while (friend != NULL && clients[client_id].no_friends < MAX_FRIENDS) {
        int friend_id = (int) strtol(friend, NULL, 10);

        // check if such friend is already on the list
        for (int i = 0; i < clients[client_id].no_friends; i++)
            if (friend_id == clients[client_id].friends[i])
                friend_id = -1;

        // if it's not on the list and its ID is valid then add to friends
        if (friend_id >= 0 && friend_id < MAX_FRIENDS && friend_id != client_id) {
            clients[client_id].friends[clients[client_id].no_friends] = friend_id;
            clients[client_id].no_friends++;
        }

        friend = strtok(NULL, FRIENDS_DELIMITER);
    }
}

// chat commands 'handlers'
void execute_stop(struct q_message *msg) {

    //shutting down only one client
    if (msg != NULL) {
        if (!is_client_available(msg->client_id)) {
            fprintf(stderr, "Failed to STOP client: %d\n", msg->client_id);
        } else {
            printf("Executing STOP for client: %d\n", msg->client_id);
            mq_close(clients[msg->client_id].queue_id);
            clients[msg->client_id].queue_id = -1;
        }
        return;
    }

    //else shutting down whole server and all clients
    printf("Executing STOP globally\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id != -1) {
            mq_close(clients[i].queue_id);
            send_message(i, STOP, "");
            kill(clients[i].pid, SIGUSR1);
        }
    }

    running = false;
}

void execute_list(struct q_message *msg) {
    printf("Executing LIST for client: %d\n", msg->client_id);
    char list[MESSAGE_LENGTH] = "";
    char buffer[MESSAGE_LENGTH];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id >= 0) {
            sprintf(buffer, "ClientID: %d\tQueueID: %d\n", i, clients[i].queue_id);
            strcat(list, buffer);
        }
    }
    send_message(msg->client_id, LIST, list);
}

void execute_init(struct q_message *msg) {
    // find the lowest available id
    int available_id;
    for (available_id = 0; available_id < MAX_CLIENTS; available_id++)
        if (clients[available_id].queue_id == -1)
            break;

    if (available_id >= MAX_CLIENTS) {
        printf("INIT failed: Clients limit is reached\n");
        return;
    }

    if ((clients[available_id].queue_id = mq_open(msg->message, O_WRONLY)) == -1)
        show_error("Error while accessing new client's queue\n");

    clients[available_id].pid = msg->client_id;
    clients[available_id].no_friends = 0;

    char initialize_message[MESSAGE_LENGTH];
    sprintf(initialize_message, "%d", available_id);
    send_message(available_id, INIT, initialize_message);
    printf("Executed INIT for new client %d with PID: %d\n", available_id, msg->client_id);
}

void execute_echo(struct q_message *msg) {
    printf("Executing ECHO for client: %d\n", msg->client_id);
    char response[MESSAGE_LENGTH];
    prepare_date_response(msg->client_id, msg->message, response);
    send_message(msg->client_id, ECHO, response);
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
        friend = strtok(friends_list, FRIENDS_DELIMITER);
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

            friend = strtok(NULL, FRIENDS_DELIMITER);
        }
    }
}

void execute_2all(struct q_message *msg) {
    printf("Executing 2ALL for client: %d\n", msg->client_id);
    char response[MESSAGE_LENGTH];
    prepare_date_response(msg->client_id, msg->message, response);

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].queue_id != -1) {
            send_message(i, _2ALL, response);
            kill(clients[i].pid, SIGUSR1);
        }
}

void execute_2friends(struct q_message *msg) {
    printf("Executing 2FRIENDS for client: %d\n", msg->client_id);
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
    printf("Executing 2ONE for client: %d\n", msg->client_id);
    int receiver;
    char text[MESSAGE_LENGTH];
    char response[MESSAGE_LENGTH];

    sscanf(msg->message, "%d %s", &receiver, text);
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

int main() {
    initialize_server();
    struct q_message buffer;
    while (running) {
        buffer.mtype = -1;
        buffer.client_id = -1;
        if (mq_receive(server_queue_id, (char *) &buffer, MSIZE, NULL) != -1)
            process_message(&buffer);
    }
    exit(3);
}
