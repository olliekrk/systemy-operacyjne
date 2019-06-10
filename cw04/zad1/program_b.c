//
// Created by olliekrk on 29.03.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

int child_alive = false;
pid_t child_pid;

void handle_SIGTSTP(int sig);

void handle_SIGINT(int sig);

void show_error(char *error_message);

void spawn_child();

int main(void) {
    struct sigaction action_SIGTSTP;

    sigemptyset(&action_SIGTSTP.sa_mask);
    action_SIGTSTP.sa_handler = handle_SIGTSTP;
    action_SIGTSTP.sa_flags = 0;

    sigaction(SIGTSTP, &action_SIGTSTP, NULL);
    if (signal(SIGINT, handle_SIGINT) == SIG_ERR)
        show_error("Unable to assign handling for SIGINT signal");

    spawn_child();

    while (true) {
        pause();
    }
}

void spawn_child() {
    fprintf(stdout, "Spawning child process\n");
    child_pid = fork();
    if (child_pid < 0)
        show_error("Creation of child process failed");

    child_alive = true;

    if (child_pid == 0) {
        execl("./date_script", "./date_script", NULL);
        show_error("Child process failed");
    }
}

//CTRL+C
void handle_SIGINT(int sig) {
    fprintf(stdout, "Received SIGINT signal. Interrupting program.\n");
    if (child_alive) {
        kill(child_pid, SIGINT);
        child_alive = false;
    }
    exit(1);
}

//CTRL+Z
void handle_SIGTSTP(int sig) {
    fprintf(stdout, "Received SIGTSTP signal.\n");
    if (child_alive) {
        child_alive = false;
        kill(child_pid, SIGINT);
        fprintf(stdout, "Killed child process with SIGINT.\n");
    } else {
        spawn_child();
    }
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}
