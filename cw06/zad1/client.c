#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/msg.h>
#include "utils6.h"

bool running = true;
int server_queue_id = -1;
int client_queue_id = -1;
int client_id = -1;

void SIGINT_handler(int sig);

void SIGUSR_handler(int sig);

int process_command(FILE *file);

// messages handling
void send_message(enum q_type type, char message[MESSAGE_LENGTH]) {
    struct q_message msg;
    msg.mtype = type;
    msg.client_id = client_id;
    strcpy(msg.message, message);
    if (msgsnd(server_queue_id, &msg, MSIZE, 0) == -1)
        show_error("Error while sending message to the server");
}

void receive_message(struct q_message *msg) {
    if (msgrcv(client_queue_id, msg, MSIZE, -(NUMBER_OF_COMMANDS), 0) == -1)
        show_error("Error while receiving response from the server");
}

// general functions & utils
void close_client() {
    if (msgctl(client_queue_id, IPC_RMID, NULL) == -1)
        show_error("Error while deleting client's queue");
    exit(0);
}

void initialize_client() {
    signal(SIGUSR1, SIGUSR_handler);
    signal(SIGINT, SIGINT_handler);

    server_queue_id = msgget(receive_server_queue_key(), 0);
    if (server_queue_id == -1) show_error("Error while opening server's queue");
    printf("Accessed server's queue with key: %d\n", server_queue_id);

    client_queue_id = msgget(receive_client_queue_key(), IPC_CREAT | IPC_EXCL | 0666);
    if (client_queue_id == -1) show_error("Error while opening client's queue");
    printf("Accessed private queue with key: %d\n", client_queue_id);
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
    char message[MESSAGE_LENGTH];
    sprintf(message, "%d", client_queue_id);
    strcpy(msg.message, message);
    msg.mtype = INIT;
    msg.client_id = getpid();

    if (msgsnd(server_queue_id, &msg, MSIZE, 0) == -1)
        show_error("Error while sending message with INIT");

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
    // mechanism alternative to notifications: informs the client to read the message from the queue
    struct q_message msg;
    receive_message(&msg);

    switch (msg.mtype) {
        case INIT:
            sscanf(msg.message, "%d", &client_id);
            break;
        case LIST:
            printf("%s\n", msg.message);
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
    initialize_client();
    atexit(close_client);
    execute_init();
    while (running)
        process_command(fdopen(STDIN_FILENO, "r"));

    exit(3);
}