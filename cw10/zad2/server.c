#include "server.h"

int main(int argc, char **argv) {
    if (argc < 3)
        show_error("Arguments must be:\n"
                   "PORT\tUNIX SOCKET PATH");

    server_port = strtol(argv[1], NULL, 10);
    if (server_port < 1024) show_error("Invalid port");

    unix_path = argv[2];
    if (strlen(unix_path) < 1 || strlen(unix_path) > UNIX_PATH_MAX) show_error("Invalid UNIX socket path");

    server_initialize();

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) < 0) show_error("epoll failed");
        message_handler(event.data.fd);
    }
}

void server_initialize() {
    atexit(server_cleanup);
    signal(SIGINT, SIGINT_handler);

    // websocket initialization
    struct sockaddr_in web_addr;
    memset(&web_addr, 0, sizeof(struct sockaddr_in)); // fill allocated memory with zeros
    web_addr.sin_family = AF_INET;
    web_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    web_addr.sin_port = htons(server_port);

    if ((web_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) show_error("Websocket creation failed");
    if (bind(web_socket_fd, (const struct sockaddr *) &web_addr, sizeof(web_addr))) show_error("Bind failed");

    // unix socket initialization
    struct sockaddr_un unix_addr;
    snprintf(unix_addr.sun_path, UNIX_PATH_MAX, "%s", unix_path);
    unix_addr.sun_family = AF_UNIX;

    if ((unix_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) show_error("UNIX socket creation failed");
    if (bind(unix_socket_fd, (const struct sockaddr *) &unix_addr, sizeof(unix_addr))) show_error("Bind failed");

    // epoll initialization
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) < 0) show_error("Epoll creation failed");
    event.data.fd = web_socket_fd;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, web_socket_fd, &event)) show_error("Epoll error");
    event.data.fd = unix_socket_fd;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, unix_socket_fd, &event)) show_error("Epoll error");

    // threads initialization
    pthread_create(&commander, NULL, commander_loop, NULL);
    pthread_detach(commander);
    pthread_create(&pinger, NULL, pinger_loop, NULL);
    pthread_detach(pinger);
}

void *commander_loop(void *args) {
    char buffer[1024];
    while (1) {
        int min_i = MAX_CLIENTS;
        int min = 1000000;

        scanf("%1023s", buffer);
        FILE *file = fopen(buffer, "r");

        if (!file) {
            printf("Failed to load file: %s\n", buffer);
            continue;
        }

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        printf("File size is %ld\n", size);
        fseek(file, 0L, SEEK_SET);

        char *file_buff = malloc(size + 1);
        if (!file_buff) {
            printf("Memory allocation for file buffer has failed\n");
            continue;
        }

        file_buff[size] = '\0';

        if (fread(file_buff, 1, size, file) != size) {
            printf("File reading has failed\n");
            free(file_buff);
            continue;
        }

        fclose(file);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!CLIENTS[i].fd) continue;
            if (min > CLIENTS[i].is_working) {
                min_i = i;
                min = CLIENTS[i].is_working;
            }
        }

        if (min_i < MAX_CLIENTS) {
            message msg = {WORK, strlen(file_buff) + 1, 0, ++ID, file_buff, NULL};
            printf("TASK NO. %lu HAS BEEN SENT TO %s\n", ID, CLIENTS[min_i].name);
            send_message(CLIENTS[min_i].fd, msg);
            CLIENTS[min_i].is_working++;
        } else {
            fprintf(stderr, "Zero clients available for task\n");
        }

        pthread_mutex_unlock(&clients_mutex);
        free(file_buff);
    }
}

void *pinger_loop(void *args) {
    while (1) {
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (CLIENTS[i].fd == 0) continue;
            if (CLIENTS[i].is_inactive) {
                delete_client(i);
            } else {
                CLIENTS[i].is_inactive = 1;
                send_empty(i, PING);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        sleep(10);
    }
}

void message_handler(int sock) {
    socklen_t addr_len = sizeof(struct sockaddr);
    struct sockaddr *addr = malloc(addr_len);

    message msg = get_message(sock, addr, &addr_len);
    pthread_mutex_lock(&clients_mutex);

    switch (msg.type) {
        case REGISTER: {
            message_type reply = OK;

            int i = get_clientID(msg.name);
            if (i < MAX_CLIENTS) reply = NAME_TAKEN;

            for (i = 0; i < MAX_CLIENTS && CLIENTS[i].fd != 0; i++);
            if (i == MAX_CLIENTS) reply = FULL;

            if (reply != OK) {
                send_empty(sock, reply);
                break;
            }

            CLIENTS[i].fd = sock;
            CLIENTS[i].name = malloc(msg.size + 1);
            if (CLIENTS[i].name == NULL) show_error("Memory allocation for client's client_name failed");
            strcpy(CLIENTS[i].name, msg.name);
            CLIENTS[i].addr = addr;
            CLIENTS[i].addr_len = addr_len;
            CLIENTS[i].is_working = 0;
            CLIENTS[i].is_inactive = 0;

            send_empty(i, reply);
            break;
        }
        case UNREGISTER: {
            int i;
            for (i = 0; i < MAX_CLIENTS && (CLIENTS[i].fd == 0 || strcmp(CLIENTS[i].name, msg.name) != 0); i++);
            if (i == MAX_CLIENTS) break;
            delete_client(i);
            break;
        }
        case WORK_DONE: {
            int i = get_clientID(msg.name);
            if (i < MAX_CLIENTS) {
                CLIENTS[i].is_inactive = 0;
                CLIENTS[i].is_working--;
            }
            printf("TASK NO. %lu COMPLETED BY %s:\n%s\n", msg.id, (char *) msg.name, (char *) msg.content);
            break;
        }
        case PONG: {
            int i = get_clientID(msg.name);
            if (i < MAX_CLIENTS)
                CLIENTS[i].is_inactive = 0;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    delete_message(msg);
}

void delete_client(int i) {
    CLIENTS[i].fd = 0;
    free(CLIENTS[i].name);
    CLIENTS[i].name = NULL;
    free(CLIENTS[i].addr);
    CLIENTS[i].addr = NULL;
    CLIENTS[i].addr_len = 0;
    CLIENTS[i].is_working = 0;
    CLIENTS[i].is_inactive = 0;
}

int get_clientID(char *client_name) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (CLIENTS[i].fd == 0) continue;
        if (strcmp(CLIENTS[i].name, client_name) == 0)
            break;
    }
    return i;
}

void delete_socket(int sock) {
    epoll_ctl(epoll, EPOLL_CTL_DEL, sock, NULL);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void send_message(int i, message msg) {
    ssize_t head_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    ssize_t size = head_size + msg.size + 1 + msg.name_size + 1;
    int8_t *buff = malloc(size);

    memcpy(buff, &msg.type, sizeof(msg.type));
    memcpy(buff + sizeof(msg.type), &msg.size, sizeof(msg.size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size), &msg.name_size, sizeof(msg.name_size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), &msg.id, sizeof(msg.id));

    if (msg.size > 0 && msg.content != NULL)
        memcpy(buff + head_size, msg.content, msg.size + 1);
    if (msg.name_size > 0 && msg.name != NULL)
        memcpy(buff + head_size + msg.size + 1, msg.name, msg.name_size + 1);

    sendto(CLIENTS[i].fd, buff, size, 0, CLIENTS[i].addr, CLIENTS[i].addr_len);
    free(buff);
}

void send_empty(int i, message_type reply) {
    message msg = {reply, 0, 0, 0, NULL, NULL};
    send_message(i, msg);
}

message get_message(int sock, struct sockaddr *addr, socklen_t *addr_len) {
    message msg;
    ssize_t header_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    int8_t buff[header_size];
    if (recv(sock, buff, header_size, MSG_PEEK) < header_size) show_error("Unknown message from client");

    memcpy(&msg.type, buff, sizeof(msg.type));
    memcpy(&msg.size, buff + sizeof(msg.type), sizeof(msg.size));
    memcpy(&msg.name_size, buff + sizeof(msg.type) + sizeof(msg.size), sizeof(msg.name_size));
    memcpy(&msg.id, buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), sizeof(msg.id));

    ssize_t size = header_size + msg.size + 1 + msg.name_size + 1;
    int8_t *buffer = malloc(size);

    if (recvfrom(sock, buffer, size, 0, addr, addr_len) < size) show_error("Unknown message from client");

    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) show_error("Invalid message content");
        memcpy(msg.content, buffer + header_size, msg.size + 1);
    } else {
        msg.content = NULL;
    }

    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
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

void SIGINT_handler(int signo) {
    exit(0);
}

void server_cleanup(void) {
    close(web_socket_fd);
    close(unix_socket_fd);
    unlink(unix_path);
    close(epoll);
}