//
// Created by olliekrk on 10.05.19.
//

#ifndef KROLIKOLGIERD_TRUCKER_H
#define KROLIKOLGIERD_TRUCKER_H

#include "common.h"

int TRUCK_LOAD = 0;
int CURRENT_TRUCK_LOAD = 0;
int LOCK_IS_MINE = 0; // used in trucker_cleanup()

int CONVEYOR_BELT_CAP = 0;
int CONVEYOR_BELT_LOAD = 0;

sem_t *belt_cap_sem = NULL;
sem_t *belt_load_sem = NULL;
sem_t *belt_lock_sem = NULL;
sem_t *loaders_sem = NULL;
sem_t *shutdown_sem = NULL;

int belt_id = -1;
conveyor_belt *belt = NULL;

int trucker_loop(int);

void trucker_unload_truck();

void trucker_load_truck();

void trucker_cleanup();

void create_conveyor_belt();

void create_semaphores();

void interrupt_handler(int);

#endif //KROLIKOLGIERD_TRUCKER_H