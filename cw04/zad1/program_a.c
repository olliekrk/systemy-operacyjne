//
// Created by olliekrk on 29.03.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <zconf.h>
#include <stdbool.h>

int child_alive = false;

void handle_SIGTSTP(int sig);

void handle_SIGINT(int sig);

void show_error(char *error_message);

int main(int argc, char **argv) {
    time_t raw_time;
    struct tm *time_info;
    struct sigaction action_SIGTSTP;

    sigemptyset(&action_SIGTSTP.sa_mask);
    action_SIGTSTP.sa_handler = handle_SIGTSTP;
    action_SIGTSTP.sa_flags = 0;

    if (signal(SIGINT, handle_SIGINT) == SIG_ERR)
        show_error("Unable to assign handling for SIGINT signal");

    while (true) {
        if (!child_alive) {
            sigaction(SIGTSTP, &action_SIGTSTP, NULL);
            time(&raw_time);
            time_info = localtime(&raw_time);
            fprintf(stdout, "%s\n", asctime(time_info));
        } else {
            pause();
        }
    }
}

void toggle_child_alive() {
    child_alive = !child_alive;
}

void handle_SIGINT(int sig) {
    fprintf(stdout, "Received SIGINT signal, interrupting program.\n");
    exit(sig);
}

void handle_SIGTSTP(int sig) {
    toggle_child_alive();
    fprintf(stdout, "Awaiting for CTRL+Z - continue or else CTRL+C - interrupt\n");
    if (signal(SIGTSTP, toggle_child_alive) == SIG_ERR)
        show_error("Unable to assign handling for SIGTSTP signal");
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}