//
// Created by olliekrk on 03.05.19.
//


#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <mqueue.h>
#include <sys/ipc.h>

#include "utils6.h"

struct q_client {
    mqd_t queue_id;
    int friends[MAX_FRIENDS];
    int no_friends;
    pid_t pid;
};

int running = true;
mqd_t server_queue_id = -1;
struct q_client clients[MAX_CLIENTS];

// chat commands 'handlers'

void process_message(struct q_message *msg);

void execute_stop(struct q_message *msg);

void execute_list(struct q_message *msg);

void execute_init(struct q_message *msg);

void execute_echo(struct q_message *msg);

void execute_friends(struct q_message *msg);

void execute_add(struct q_message *msg);

void execute_del(struct q_message *msg);

void execute_2all(struct q_message *msg);

void execute_2friends(struct q_message *msg);

void execute_2one(struct q_message *msg);

//

bool is_client_available(int client_id) {
    return client_id < MAX_CLIENTS && client_id >= 0 && clients[client_id].queue_id != -1;
}

void close_server() {
    if (mq_close(server_queue_id) == -1 || mq_unlink(SERVER_QUEUE_NAME) == -1)
        show_error("Error while deleting queue");
    exit(0);
}

void SIGINT_handler(int sig) {
    execute_stop(NULL);
    exit(0);
//    close_server();
}

// server loop

int main() {
    atexit(close_server);
    signal(SIGINT, SIGINT_handler);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].queue_id = -1;
        clients[i].no_friends = 0;
    }

    struct mq_attr queue_attr;
    queue_attr.mq_maxmsg = QUEUE_SIZE;
    queue_attr.mq_msgsize = MESSAGE_LENGTH;

    server_queue_id = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT | O_EXCL, 0666, &queue_attr);
    if (server_queue_id == -1) {
        show_error("Error while creating server queue");
    } else {
        printf("Accessed server's queue\n");
    }

    struct q_message buffer;

    while (running) {
        buffer.mtype = -1;
        buffer.client_id = -1;
        printf("Awaiting for incoming messages...\n");
        mq_receive(server_queue_id, (char *) &buffer, MESSAGE_LENGTH, NULL);
        printf("Received a message. Processing...\n");
        process_message(&buffer);
    }
    return 0;
}

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

int send_message(int client_id, enum q_type type, char message[MESSAGE_LENGTH]) {
    struct q_message response;
    response.mtype = type;
    response.client_id = client_id;
    strcpy(response.message, message);

    if (is_client_available(client_id)) {
        if (mq_send(clients[client_id].queue_id, (char *) &response, MESSAGE_LENGTH, message_priority(type)) == -1)
            show_error("Error while sending response message");
        return 0;
    } else {
        fprintf(stderr, "Invalid client ID - response has not been sent\n");
        return -1;
    }
}

// 'handlers' implementation

void add_friends(pid_t client_id, char friends_list[MESSAGE_LENGTH]) {
    char *friend = strtok(friends_list, (const char *) FRIENDS_DELIMITER);
    while (friend != NULL && clients[client_id].no_friends < MAX_CLIENTS) {

        int friend_id = (int) strtol(friend, NULL, 10);

        for (int i = 0; i < clients[client_id].no_friends; i++) {
            if (friend_id == clients[client_id].friends[i])
                friend_id = -1;
        }

        if (friend_id < MAX_CLIENTS && friend_id >= 0 && friend_id != client_id) {
            clients[client_id].friends[clients[client_id].no_friends] = friend_id;
            clients[client_id].no_friends++;
        }

        friend = strtok(NULL, (const char *) FRIENDS_DELIMITER);
    }
}

void execute_stop(struct q_message *msg) {
    if (msg != NULL && msg->client_id >= 0) {
        mq_close(clients[msg->client_id].queue_id);
        clients[msg->client_id].queue_id = -1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id >= 0) {
            send_message(i, STOP, "");
            mq_close(clients[i].queue_id);
            kill(clients[i].pid, SIGUSR1);
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id >= 0) {
            struct q_message msg;
            mq_receive(server_queue_id, (char *) &msg, MESSAGE_LENGTH, NULL);
        }
    }
    running = false;
}

void execute_list(struct q_message *msg) {
    char response_message[MESSAGE_LENGTH];
    char buffer[MESSAGE_LENGTH];
    strcpy(response_message, "");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].queue_id >= 0) {
            sprintf(buffer, "Client's ID: %d\tClient Queue's ID:%d\n", i, clients[i].queue_id);
            strcat(response_message, buffer);
        }
    }
    send_message(msg->client_id, LIST, response_message);
}

void execute_init(struct q_message *msg) {
    int i_id;
    // find the lowest unused id
    for (i_id = 0; i_id < MAX_CLIENTS; i_id++) {
        if (clients[i_id].queue_id == -1)
            break;
    }
    if (i_id >= MAX_CLIENTS) {
        printf("Clients limit is reached");
        return;
    }

    if ((clients[i_id].queue_id = mq_open(msg->message, O_WRONLY)) == -1) {
        show_error("Error while creating queue\n");
    }

    clients[i_id].pid = msg->client_id;
    clients[i_id].no_friends = 0;

    printf("Initialized client with queue: %d and ID: %d\n", clients[i_id].queue_id, i_id);

    char initialize_message[MESSAGE_LENGTH];
    sprintf(initialize_message, "%d", i_id);
    send_message(i_id, INIT, initialize_message);
}

void execute_echo(struct q_message *msg) {
    if (msg == NULL)
        return;

    char response_message[MESSAGE_LENGTH];
    char datetime[31];

    FILE *date = popen("date", "r");
    fread(datetime, sizeof(char), 31, date);
    pclose(date);

    sprintf(response_message, "%s\t%s", msg->message, datetime);
    send_message(msg->client_id, ECHO, response_message);
}

void execute_friends(struct q_message *msg) {
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
    if (!is_client_available(msg->client_id)) {
        fprintf(stderr, "Invalid client ID: %d\n", msg->client_id);
        return;
    }

    char friends_list[MESSAGE_LENGTH];
    int n = sscanf(msg->message, "%s", friends_list);
    char *friend;
    int friend_id;
    int *no_friends = &clients[msg->client_id].no_friends;

    if (n == EOF || n == 0) {
        fprintf(stderr, "Error while deleting friends of client %d\n", msg->client_id);
        return;
    }
    if (n == 1) {
        friend = strtok(friends_list, (const char *) FRIENDS_DELIMITER);
        int i;
        while (friend != NULL && (*no_friends) > 0) {
            friend_id = strtol(friend, NULL, 10);

            for (i = 0; i < (*no_friends); i++)
                if (friend_id == clients[msg->client_id].friends[i])
                    break;
            if (i >= (*no_friends))
                friend_id = -1;
            if (friend_id < MAX_CLIENTS && friend_id >= 0 && friend_id != msg->client_id) {
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
        send_message(receiver, _2FRIENDS, response);
        kill(clients[receiver].pid, SIGUSR1);
    }
}