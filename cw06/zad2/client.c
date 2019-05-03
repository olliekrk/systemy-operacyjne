#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <mqueue.h>
#include <sys/ipc.h>

#include "utils6.h"

bool running = true;
char *queue_name;
int server_queue_id = -1;
int client_queue_id = -1;
int client_id = -1;

void send_message(enum q_type type, char message[MESSAGE_LENGTH]) {
    struct q_message msg;
    msg.mtype = type;
    msg.client_id = client_id;
    strcpy(msg.message, message);

    if (mq_send(server_queue_id, (char *) &msg, MESSAGE_LENGTH, message_priority(type)) == -1)
        show_error("Error while sending message to the server");
}

void receive_message(struct q_message *msg) {
    printf("Awaiting for server's response...\n");
    if (mq_receive(client_queue_id, (char *) msg, MESSAGE_LENGTH, NULL) == -1)
        show_error("Error while receiving response from the server");
    printf("Received response!\n");
}

int process_command(FILE *file);

void close_client() {
    mq_close(server_queue_id);
    mq_close(client_queue_id);
    mq_unlink(queue_name);
    free(queue_name);
}

// client commands 'handlers'

void execute_init() {
    struct q_message msg;
    msg.mtype = INIT;
    msg.client_id = getpid();
    strcpy(msg.message, queue_name);

    if (mq_send(server_queue_id, (char *) &msg, MESSAGE_LENGTH, message_priority(INIT)) == -1) {
        show_error("Error while sending message");
    }
    receive_message(&msg);

    sscanf(msg.message, "%d", &client_id);
}

void execute_stop() {
    running = false;
    send_message(STOP, "");
    exit(0);
}

void execute_read(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char filename[COMMAND_LENGTH];

    int no_args = sscanf(argv, "%s %s", command, filename);
    if (no_args == EOF || no_args < 2) {
        fprintf(stderr, "Missing file name argument\n");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (!file)
        show_error("Error while opening file");

    while (process_command(file) != EOF);
    fclose(file);
}

void execute_echo(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc < 2) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(ECHO, message);
    struct q_message msg;
    receive_message(&msg);
    if (msg.mtype != ECHO)
        show_error("Invalid response type from the server");
    printf("%s\n", msg.message);
}

void execute_list() {
    send_message(LIST, "");
    struct q_message msg;
    receive_message(&msg);
    if (msg.mtype != LIST)
        show_error("Invalid response type from the server");
    printf("%s\n", msg.message);
}

void execute_friends(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc == 0) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(FRIENDS, message);
}

void execute_add(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc < 2) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(ADD, message);
}

void execute_del(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc < 2) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(DEL, message);
}

void execute_2all(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc < 2) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(_2ALL, message);
}

void execute_2one(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int receiver;
    int argc = sscanf(argv, "%s %d %s", command, &receiver, message);
    if (argc == EOF || argc < 3) {
        printf("Invalid number of arguments");
        return;
    }
    sprintf(command, "%d %s", receiver, message);
    send_message(_2ONE, command);
}

void execute_2friends(char argv[COMMAND_LENGTH]) {
    char command[COMMAND_LENGTH];
    char message[MESSAGE_LENGTH];
    int argc = sscanf(argv, "%s %s", command, message);
    if (argc == EOF || argc < 2) {
        printf("Invalid number of arguments");
        return;
    }
    send_message(_2FRIENDS, message);
}

// signal handlers

void SIGINT_handler(int sig) {
    execute_stop();
    exit(0);
//    close_client();
}

void SIGUSR_handler(int sig) {
    struct q_message msg;
    receive_message(&msg);
    switch (msg.mtype) {
        case _2ONE:
        case _2FRIENDS:
        case _2ALL:
            printf("%s", msg.message);
            break;
        case STOP:
            execute_stop();
            break;
    }
}

// client loop
int process_command(FILE *file) {
    char argv[COMMAND_LENGTH];
    char command[COMMAND_LENGTH];

    if (fgets(argv, COMMAND_LENGTH * sizeof(char), file) == NULL)
        return EOF;

    int argc = sscanf(argv, "%s", command);
    if (argc == EOF || argc == 0)
        return 0;

    if (strcmp(command, "ECHO") == 0) {
        execute_echo(argv);
    } else if (strcmp(command, "LIST") == 0) {
        execute_list();
    } else if (strcmp(command, "FRIENDS") == 0) {
        execute_friends(argv);
    } else if (strcmp(command, "2ALL") == 0) {
        execute_2all(argv);
    } else if (strcmp(command, "2FRIENDS") == 0) {
        execute_2friends(argv);
    } else if (strcmp(command, "2ONE") == 0) {
        execute_2one(argv);
    } else if (strcmp(command, "STOP") == 0) {
        execute_stop();
        return EOF;
    } else if (strcmp(command, "READ") == 0) {
        execute_read(argv);
    } else if (strcmp(command, "ADD") == 0) {
        execute_add(argv);
    } else if (strcmp(command, "DEL") == 0) {
        execute_del(argv);
    } else {
        printf("Invalid command\n");
    }
    return 0;
}

int main() {
    atexit(close_client);
    signal(SIGUSR1, SIGUSR_handler);
    signal(SIGINT, SIGINT_handler);

    queue_name = receive_client_queue_name();

    server_queue_id = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_queue_id == -1)
        show_error("Error while opening server's queue");

    struct mq_attr queue_attr;
    queue_attr.mq_maxmsg = QUEUE_SIZE;
    queue_attr.mq_msgsize = MESSAGE_LENGTH;

    client_queue_id = mq_open(queue_name, O_RDONLY | O_CREAT | O_EXCL, 0666, &queue_attr);
    if (client_queue_id == -1)
        show_error("Error while opening client's queue");

    printf("Accessed server's queue with key: %d\n", server_queue_id);
    printf("Accessed private queue with key: %d\n", client_queue_id);
    execute_init();

    while (running) {
        process_command(fdopen(STDIN_FILENO, "r"));
    }
    return 0;
}