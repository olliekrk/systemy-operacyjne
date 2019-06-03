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
            case OK: {
                break;
            }
            case PING: {
                send_empty(PONG);
                break;
            }
            case NAME_TAKEN:
                show_error("Name is already taken");
            case FULL:
                show_error("Server is full");
            case WORK: {
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
            }
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

        struct sockaddr_in web_addr;
        memset(&web_addr, 0, sizeof(struct sockaddr_in));
        web_addr.sin_family = AF_INET;
        web_addr.sin_addr.s_addr = in_addr;
        web_addr.sin_port = htons(port_num);

        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) show_error("Socket creation failed");
        if (connect(socket_fd, (const struct sockaddr *) &web_addr, sizeof(web_addr))) show_error("Connect has failed");

    } else if (strcmp("UNIX", type) == 0) {
        char *un_path = address;

        if (strlen(un_path) < 1 || strlen(un_path) > MAX_UNIXPATH) show_error("Invalid UNIX socket path");

        struct sockaddr_un un_addr;
        un_addr.sun_family = AF_UNIX;
        snprintf(un_addr.sun_path, MAX_UNIXPATH, "%s", un_path);

        if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0))) show_error("Socket creation failed");

        if (connect(socket_fd, (const struct sockaddr *) &un_addr, sizeof(un_addr))) show_error("Connect has failed");
    } else {
        show_error("Invalid connection type argument");
    }

    send_empty(REGISTER);
}

void send_msg(message *msg) {
    write(socket_fd, &msg->type, sizeof(msg->type));
    write(socket_fd, &msg->size, sizeof(msg->size));
    write(socket_fd, &msg->name_size, sizeof(msg->name_size));
    write(socket_fd, &msg->id, sizeof(msg->id));
    if (msg->size > 0) write(socket_fd, msg->content, msg->size);
    if (msg->name_size > 0) write(socket_fd, msg->name, msg->name_size);
}

void send_empty(message_type type) {
    message msg = {type, 0, strlen(client_name) + 1, 0, NULL, client_name};
    send_msg(&msg);
}

void send_done(int id, char *content) {
    message msg = {WORK_DONE, strlen(content) + 1, strlen(client_name) + 1, id, content, client_name};
    send_msg(&msg);
}

message get_message(void) {
    message msg;
    if (read(socket_fd, &msg.type, sizeof(msg.type)) != sizeof(msg.type))
        show_error("Unknown message from server");
    if (read(socket_fd, &msg.size, sizeof(msg.size)) != sizeof(msg.size))
        show_error("Unknown message from server");
    if (read(socket_fd, &msg.name_size, sizeof(msg.name_size)) != sizeof(msg.name_size))
        show_error("Unknown message from server");
    if (read(socket_fd, &msg.id, sizeof(msg.id)) != sizeof(msg.id))
        show_error("Unknown message from server");
    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) show_error("Invalid message content");
        if (read(socket_fd, msg.content, msg.size) != msg.size) show_error("Invalid message content");
    } else {
        msg.content = NULL;
    }
    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
        if (msg.name == NULL) show_error("Invalid message content");
        if (read(socket_fd, msg.name, msg.name_size) != msg.name_size) show_error("Invalid message content");
    } else {
        msg.name = NULL;
    }
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
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}