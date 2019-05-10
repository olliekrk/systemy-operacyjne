//
// Created by olliekrk on 11.05.19.
//

#ifndef KROLIKOLGIERD_LOADER_H
#define KROLIKOLGIERD_LOADER_H

#include "common.h"

void loader_loop();

void loader_cleanup();

void access_conveyor_belt();

void access_semaphores();

void interrupt_handler(int);


#endif //KROLIKOLGIERD_LOADER_H
