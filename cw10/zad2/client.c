//
// Created by olliekrk on 31.05.19.
//
#include "client.h"

int main(int argc, char **argv) {
    if (argc < 4)
        show_error("Arguments must be:\n"
                   "NAME\tCONNECTION TYPE\tIP + PORT or UNIX SOCKET");

    client_name = argv[1];
    char *type_arg = argv[2];
    char *address_arg = argv[3];

    client_initialize(type_arg, address_arg);

    while (1) {
        message msg = get_message();
        switch (msg.type) {
            case OK:
                break;
            case PING:
                send_empty(PONG);
                break;
            case NAME_TAKEN:
                show_error("Name is already taken");
            case FULL:
                show_error("Server is full");
            case WORK:
                printf("Started new task...");
                char *buffer = malloc(100 + 2 * msg.size);
                if (buffer == NULL) show_error("Memory allocation failure");
                sprintf(buffer, "echo '%s' | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c", (char *) msg.content);
                FILE *result = popen(buffer, "r");
                if (result == 0) {
                    free(buffer);
                    break;
                }
                int n = fread(buffer, 1, 99 + 2 * msg.size, result);
                buffer[n] = '\0';
                printf("Task has been finished.");
                send_done(msg.id, buffer);
                free(buffer);
                break;
            default:
                break;
        }
        delete_message(msg);
    }
}

void client_initialize(char *type, char *address) {
    atexit(client_cleanup);
    signal(SIGINT, SIGINT_handler);

    if (strcmp("WEB", type) == 0) {
        strtok(address, ":");
        char *port = strtok(NULL, ":");
        if (!port) show_error("Port was not specified");

        uint32_t in_addr = inet_addr(address);
        if (in_addr == INADDR_NONE) show_error("Invalid address argument");

        uint16_t port_num = strtol(port, NULL, 10);
        if (port_num < 1024) show_error("Invalid port number");

        if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) show_error("Socket creation failed");

        struct sockaddr_in web_addr;
        memset(&web_addr, 0, sizeof(struct sockaddr_in));
        web_addr.sin_family = AF_INET;
        web_addr.sin_addr.s_addr = in_addr;
        web_addr.sin_port = htons(port_num);

        if (connect(socket_fd, (const struct sockaddr *) &web_addr, sizeof(web_addr))) show_error("Connect has failed");

    } else if (strcmp("UNIX", type) == 0) {
        char *un_path = address;

        if (strlen(un_path) < 1 || strlen(un_path) > UNIX_PATH_MAX) show_error("Invalid UNIX socket path");

        struct sockaddr_un unix_addr;
        unix_addr.sun_family = AF_UNIX;
        snprintf(unix_addr.sun_path, UNIX_PATH_MAX, "%s", un_path);

        struct sockaddr_un unix_client_addr;
        memset(&unix_client_addr, 0, sizeof(unix_client_addr));
        unix_client_addr.sun_family = AF_UNIX;
        snprintf(unix_client_addr.sun_path, UNIX_PATH_MAX, "%s", client_name);

        if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
            show_error("Socket creation failed");
        if (bind(socket_fd, (const struct sockaddr *) &unix_client_addr, sizeof(unix_client_addr)) < 0)
            show_error("Client bind has failed");
        if (connect(socket_fd, (const struct sockaddr *) &unix_addr, sizeof(unix_addr)) < 0)
            show_error("Connect has failed");
    } else {
        show_error("Invalid connection type argument");
    }

    send_empty(REGISTER);
}

void send_message(message msg) {
    ssize_t header_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    ssize_t size = header_size + msg.size + 1 + msg.name_size + 1;
    int8_t *buff = malloc(size);
    if (buff == NULL) show_error("Memory allocation failure");

    memcpy(buff, &msg.type, sizeof(msg.type));
    memcpy(buff + sizeof(msg.type), &msg.size, sizeof(msg.size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size), &msg.name_size, sizeof(msg.name_size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), &msg.id, sizeof(msg.id));

    if (msg.size > 0 && msg.content != NULL)
        memcpy(buff + header_size, msg.content, msg.size + 1);
    if (msg.name_size > 0 && msg.name != NULL)
        memcpy(buff + header_size + msg.size + 1, msg.name, msg.name_size + 1);

    if (write(socket_fd, buff, size) != size) show_error("Write to connection has failed");

    free(buff);
}

void send_empty(message_type type) {
    message msg = {type, 0, strlen(client_name), 0, NULL, client_name};
    send_message(msg);
}

void send_done(int id, char *content) {
    message msg = {WORK_DONE, strlen(content), strlen(client_name), id, content, client_name};
    send_message(msg);
}

message get_message(void) {
    message msg;
    ssize_t header_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    int8_t buff[header_size];
    if (recv(socket_fd, buff, header_size, MSG_PEEK) < header_size)
        show_error("Too short header received");

    memcpy(&msg.type, buff, sizeof(msg.type));
    memcpy(&msg.size, buff + sizeof(msg.type), sizeof(msg.size));
    memcpy(&msg.name_size, buff + sizeof(msg.type) + sizeof(msg.size), sizeof(msg.name_size));
    memcpy(&msg.id, buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), sizeof(msg.id));

    ssize_t size = header_size + msg.size + 1 + msg.name_size + 1;
    int8_t *buffer = malloc(size);

    if (recv(socket_fd, buffer, size, 0) < size) show_error("Too short message received");

    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) show_error("Unknown message from server");
        memcpy(msg.content, buffer + header_size, msg.size + 1);
    } else {
        msg.content = NULL;
    }

    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
        if (msg.name == NULL) show_error("Unknown message from server");
        memcpy(msg.name, buffer + header_size + msg.size + 1, msg.name_size + 1);
    } else {
        msg.name = NULL;
    }

    free(buffer);
    return msg;
}

void delete_message(message msg) {
    if (msg.content != NULL) free(msg.content);
    if (msg.name != NULL) free(msg.name);
}

void SIGINT_handler(int sig) {
    exit(0);
}

void client_cleanup(void) {
    send_empty(UNREGISTER);
    unlink(client_name);
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}