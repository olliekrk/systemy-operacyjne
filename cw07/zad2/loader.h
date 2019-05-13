//
// Created by olliekrk on 11.05.19.
//

#ifndef KROLIKOLGIERD_LOADER_H
#define KROLIKOLGIERD_LOADER_H

#include "common.h"

int ITEM_WEIGHT = 0;
int NUMBER_OF_CYCLES = -1; // infinity by default

sem_t *belt_cap_sem = NULL;
sem_t *belt_load_sem = NULL;
sem_t *belt_lock_sem = NULL;
sem_t *loaders_sem = NULL;

int belt_id = -1;
conveyor_belt *belt = NULL;

void loader_loop();

void loader_load_belt(belt_item);

void loader_cleanup();

void access_conveyor_belt();

void access_semaphores();

void interrupt_handler(int);

#endif //KROLIKOLGIERD_LOADER_H