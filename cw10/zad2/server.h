//
// Created by olliekrk on 03.06.19.
//

#ifndef KROLIKOLGIERD_SERVER_H
#define KROLIKOLGIERD_SERVER_H

#include "socketman.h"

uint16_t server_port;
int unix_socket_fd;
int web_socket_fd;

int epoll;
char *unix_path;

uint64_t ID;

client CLIENTS[MAX_CLIENTS];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t pinger;
pthread_t commander;

void server_initialize();

void *commander_loop(void *);

void *pinger_loop(void *);

void message_handler(int);

void send_message(int, message);

void send_empty(int, message_type);

void delete_client(int);

void delete_socket(int);

int get_clientID(char *);

message get_message(int, struct sockaddr *, socklen_t *);

void delete_message(message);

void SIGINT_handler(int);

void server_cleanup(void);

#endif //KROLIKOLGIERD_SERVER_H
