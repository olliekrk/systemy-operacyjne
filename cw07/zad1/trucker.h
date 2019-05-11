//
// Created by olliekrk on 10.05.19.
//

#ifndef KROLIKOLGIERD_TRUCKER_H
#define KROLIKOLGIERD_TRUCKER_H

#include "common.h"

#define CREATION_FLAG IPC_CREAT | 0666 /*| IPC_EXCL*/

// truck related
int current_truck_load;
int TRUCK_LOAD = 0;

// belt related
int CONVEYOR_BELT_CAP = 0;
int CONVEYOR_BELT_LOAD = 0;

// IDs
int sem_id = -1;
int belt_id = -1;
conveyor_belt *belt = NULL;

int trucker_loop(int);

void trucker_unload_truck(int);

void trucker_load_truck();

void trucker_cleanup();

void create_conveyor_belt();

void create_semaphores();

void interrupt_handler(int);

#endif //KROLIKOLGIERD_TRUCKER_H