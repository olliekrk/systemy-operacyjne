//
// Created by olliekrk on 10.05.19.
//

#ifndef KROLIKOLGIERD_TRUCKER_H
#define KROLIKOLGIERD_TRUCKER_H

#include "common.h"

void trucker_loop();

void trucker_cleanup();

void create_conveyor_belt();

void create_semaphores();

void create_loaders(int);

void interrupt_handler(int);

void print_factory_event(factory_event*);

#endif //KROLIKOLGIERD_TRUCKER_H
