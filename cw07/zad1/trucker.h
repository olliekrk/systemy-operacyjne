//
// Created by olliekrk on 10.05.19.
//

#ifndef KROLIKOLGIERD_TRUCKER_H
#define KROLIKOLGIERD_TRUCKER_H

#include "common.h"

int trucker_loop();

void trucker_cleanup();

void create_conveyor_belt();

void create_semaphores();

void interrupt_handler(int);


#endif //KROLIKOLGIERD_TRUCKER_H
