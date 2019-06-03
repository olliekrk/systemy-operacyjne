//
// Created by olliekrk on 31.05.19.
//

#ifndef KROLIKOLGIERD_SOCKETMAN_H
#define KROLIKOLGIERD_SOCKETMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#define MAX_CLIENTS 16
#define MAX_CLIENTNAME 20
#define MAX_UNIXPATH 100
#define MAX_BACKLOG 64

typedef enum message_type {
    REGISTER,
    UNREGISTER,
    PING,
    PONG,
    OK,
    NAME_TAKEN,
    FULL,
    FAIL,
    WORK,
    WORK_DONE,
} message_type;

typedef struct message {
    uint8_t type;
    uint64_t size;
    uint64_t name_size;
    uint64_t id;
    void *content;
    char *name;
} message;

typedef struct client {
    int fd;
    char *name;
    uint8_t working;
    uint8_t inactive;
} client;

void show_error(const char *message) {
    perror(message);
    exit(1);
}

typedef struct timeval tv;

void print_time(tv *time) {
    printf("%ld s, %ld us\n", time->tv_sec, time->tv_usec);
}

void get_current_time(tv *buffer) {
    gettimeofday(buffer, NULL);
}

tv *get_time_difference(tv *a, tv *b) { // result = a - b
    tv *result = calloc(1, sizeof(tv));

    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;
    if ((result)->tv_usec < 0) {
        --(result)->tv_sec;
        (result)->tv_usec += 1000000;
    }
    return result;
}

#endif //KROLIKOLGIERD_SOCKETMAN_H

/*
 * Klient i serwer muszą sobie utworzyć socket()
 * bind() wykonany po stronie serwera, związanie gniazda z jego adresem
 * listen() sewrer: nasłuchiwanie na porcie i limit klientów
 * connect() wywoływana przez klienta
 * serwer akceptuje accept()
 * wymiana informacji następuje przy pomocy send/read/write/receive etc
 *
 * na koniec:
 * shutdown() po obu stronach
 * close() zamykajace deskryptor po obu stronach
 *
 * W bezpołączeniowych UDP :
 * nie ma listen()
 * nie ma shutdown, tylko close
 *
 * W unixowych:
 * po stronie klienta też bind()
 * nie ma shutdown, tylko close
 *
 * */
