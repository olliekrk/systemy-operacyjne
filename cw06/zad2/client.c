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
char *client_queue_name;
int server_queue_id = -1;
int client_queue_id = -1;
int client_id = -1;

int process_command(FILE *file);

// messages handling
void send_message(enum q_type type, char message[MESSAGE_LENGTH]) {
    struct q_message msg;
    msg.mtype = type;
    msg.client_id = client_id;
    strcpy(msg.message, message);
    if (mq_send(server_queue_id, (char *) &msg, MSIZE, message_priority(type)) == -1)
        show_error("Error while sending message to the server");
}

void receive_message(struct q_message *msg) {
    if (mq_receive(client_queue_id, (char *) msg, MSIZE, NULL) == -1)
        show_error("Error while receiving response from the server");
}

// general functions & utils
void close_client() {
    mq_close(server_queue_id);
    mq_close(client_queue_id);
    if (mq_unlink(client_queue_name) == -1)
        show_error("Error while deleting client's queue");
    free(client_queue_name);
    exit(0);
}

int extract_1argument(char *argv, char *arg1) {
    char command[COMMAND_LENGTH];
    char args[COMMAND_LENGTH];
    int no_args = sscanf(argv, "%s %s", command, args);
    if (no_args == EOF || no_args < 2) {
        fprintf(stderr, "Missing command argument(s)\n");
        return -1;
    }
    strcpy(arg1, args);
    return 0;
}

// client commands 'handlers'
void execute_init() {
    printf("Executing INIT command.\n");
    struct q_message msg;
    strcpy(msg.message, client_queue_name);
    msg.mtype = INIT;
    msg.client_id = getpid();

    if (mq_send(server_queue_id, (char *) &msg, MSIZE, message_priority(INIT)) == -1)
        show_error("Error while sending message");

    raise(SIGUSR1);
}

void execute_stop() {
    printf("Executing STOP command.\n");
    send_message(STOP, "");
    running = false;
    exit(3);
}

void execute_list() {
    printf("Executing LIST command.\n");
    send_message(LIST, "");
    raise(SIGUSR1);
}

void execute_read(char argv[COMMAND_LENGTH]) {
    printf("Executing READ command.\n");
    char filename[COMMAND_LENGTH];
    if (extract_1argument(argv, filename) == -1) return;

    FILE *file = fopen(filename, "r");
    if (!file)
        show_error("Error while opening file");

    while (process_command(file) != EOF);
    fclose(file);
}

void execute_echo(char argv[COMMAND_LENGTH]) {
    printf("Executing ECHO command.\n");
    char message[MESSAGE_LENGTH];
    if (extract_1argument(argv, message) == -1) return;

    send_message(ECHO, message);
    raise(SIGUSR1);
}

void execute_friends(char argv[COMMAND_LENGTH]) {
    printf("Executing FRIENDS command.\n");
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
    printf("Executing ADD command.\n");
    char message[MESSAGE_LENGTH];
    if (extract_1argument(argv, message) == -1) return;
    send_message(ADD, message);
}

void execute_del(char argv[COMMAND_LENGTH]) {
    printf("Executing DEL command.\n");
    char message[MESSAGE_LENGTH];
    if (extract_1argument(argv, message) == -1) return;
    send_message(DEL, message);
}

void execute_2all(char argv[COMMAND_LENGTH]) {
    printf("Executing 2ALL command.\n");
    char message[MESSAGE_LENGTH];
    if (extract_1argument(argv, message) == -1) return;
    send_message(_2ALL, message);
}

void execute_2friends(char argv[COMMAND_LENGTH]) {
    printf("Executing 2FRIENDS command.\n");
    char message[MESSAGE_LENGTH];
    if (extract_1argument(argv, message) == -1) return;
    send_message(_2FRIENDS, message);
}

void execute_2one(char argv[COMMAND_LENGTH]) {
    printf("Executing 2ONE command.\n");
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

// signal handlers
void SIGINT_handler(int sig) {
    printf("Signal SIGINT received.\n");
    execute_stop();
}

void SIGUSR_handler(int sig) {
    struct q_message msg;
    receive_message(&msg);

    switch (msg.mtype) {
        case INIT:
            sscanf(msg.message, "%d", &client_id);
            printf("Initialized as client: %d with queue: %d named: %s\n",
                   client_id, client_queue_id, client_queue_name);
            break;
        case LIST:
            printf("%s", msg.message);
            break;
        case ECHO:
            printf("[ECHO]: %s", msg.message);
            break;
        case _2ONE:
            printf("[PRIVATE]: %s", msg.message);
            break;
        case _2FRIENDS:
            printf("[FRIENDS]: %s", msg.message);
            break;
        case _2ALL:
            printf("[ALL]: %s", msg.message);
            break;
        case STOP:
            printf("Executing global STOP command.\n");
            exit(3);
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

    if (strcmp(command, "ECHO") == 0)
        execute_echo(argv);
    else if (strcmp(command, "LIST") == 0)
        execute_list();
    else if (strcmp(command, "FRIENDS") == 0)
        execute_friends(argv);
    else if (strcmp(command, "2ALL") == 0)
        execute_2all(argv);
    else if (strcmp(command, "2FRIENDS") == 0)
        execute_2friends(argv);
    else if (strcmp(command, "2ONE") == 0)
        execute_2one(argv);
    else if (strcmp(command, "READ") == 0)
        execute_read(argv);
    else if (strcmp(command, "ADD") == 0)
        execute_add(argv);
    else if (strcmp(command, "DEL") == 0)
        execute_del(argv);
    else if (strcmp(command, "STOP") == 0) {
        execute_stop();
        return EOF;
    } else
        printf("Invalid command\n");

    return 0;
}

int main() {
    signal(SIGUSR1, SIGUSR_handler);
    signal(SIGINT, SIGINT_handler);
    atexit(close_client);

    server_queue_id = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_queue_id == -1) show_error("Error while opening server's queue");
    printf("Accessed server's queue with key: %d\n", server_queue_id);

    struct mq_attr queue_attr;
    queue_attr.mq_maxmsg = QUEUE_SIZE;
    queue_attr.mq_msgsize = MSIZE;

    client_queue_name = receive_client_queue_name();
    client_queue_id = mq_open(client_queue_name, O_RDONLY | O_CREAT | O_EXCL, 0666, &queue_attr);
    if (client_queue_id == -1) show_error("Error while opening client's queue");
    printf("Accessed private queue with key: %d\n", client_queue_id);

    execute_init();
    while (running)
        process_command(fdopen(STDIN_FILENO, "r"));

    exit(3);
}