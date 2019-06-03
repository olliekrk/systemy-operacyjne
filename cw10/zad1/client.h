//
// Created by olliekrk on 03.06.19.
//

#ifndef KROLIKOLGIERD_CLIENT_H
#define KROLIKOLGIERD_CLIENT_H

#include <arpa/inet.h>
#include "socketman.h"

int socket_fd;
char *client_name;

void client_initialize(char *, char *);

message get_message(void);

void delete_message(message);

void send_message(message *);

void send_empty(message_type);

void send_done(int, char *);

void SIGINT_handler(int);

void client_cleanup(void);

#endif //KROLIKOLGIERD_CLIENT_H
