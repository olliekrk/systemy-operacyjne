//
// Created by olliekrk on 03.04.19.
//

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void receive_KILL(int sig, siginfo_t *info, void *context);

static void receive_SIGQUEUE(int sig, siginfo_t *info, void *context);

static void receive_SIGRT(int sig, siginfo_t *info, void *context);

void modify_SIGUSR_behaviour(void (*receiver)(int, siginfo_t *, void *));

void modify_SIGRT_behaviour(void (*receiver)(int, siginfo_t *, void *));

void show_error(char *error_message);

int actual_no_signals = 0;

int main(int argc, char **argv) {

    char *mode = argv[1];

    if (strcmp(mode, "KILL") == 0) {
        modify_SIGUSR_behaviour(receive_KILL);
    } else if (strcmp(mode, "SIGQUEUE") == 0) {
        modify_SIGUSR_behaviour(receive_SIGQUEUE);
    } else if (strcmp(mode, "SIGRT") == 0) {
        modify_SIGRT_behaviour(receive_SIGRT);
    } else {
        show_error("Unknown mode");
    }

    printf("Catcher stated with PID: %d\n", getpid());
    while (1) pause();
}

static void receive_KILL(int sig, siginfo_t *info, void *context) {
    switch (sig) {
        case SIGUSR1:
            actual_no_signals++;
            break;
        case SIGUSR2:
            for (int i = 0; i < actual_no_signals; i++)
                kill(info->si_pid, SIGUSR1);
            kill(info->si_pid, SIGUSR2);
        default:
            exit(0);
    }
}

static void receive_SIGQUEUE(int sig, siginfo_t *info, void *context) {
    switch (sig) {
        case SIGUSR1:
            actual_no_signals++;
            break;
        case SIGUSR2:
            for (int i = 0; i < actual_no_signals; i++) {
                union sigval ith_signal = {i};
                sigqueue(info->si_pid, SIGUSR1, ith_signal);
            }
            kill(info->si_pid, SIGUSR2);
        default:
            exit(0);
    }
}

static void receive_SIGRT(int sig, siginfo_t *info, void *context) {
    if (sig == SIGRTMIN) {
        actual_no_signals++;
    } else if (sig == SIGRTMAX) {
        for (int i = 0; i < actual_no_signals; i++)
            kill(info->si_pid, SIGRTMIN);
        kill(info->si_pid, SIGRTMAX);
        exit(0);
    }
}

void modify_SIGUSR_behaviour(void (*receiver)(int, siginfo_t *, void *)) {
    sigset_t activated_signals;

    //block every but sigusr1 and sigusr2
    sigfillset(&activated_signals);
    sigdelset(&activated_signals, SIGUSR1);
    sigdelset(&activated_signals, SIGUSR2);

    //to store blocked signals until we unlock them
    sigprocmask(SIG_BLOCK, &activated_signals, NULL);

    struct sigaction action_SIGUSR;
    action_SIGUSR.sa_sigaction = receiver;
    action_SIGUSR.sa_flags = SA_SIGINFO;
    sigemptyset(&action_SIGUSR.sa_mask);

    sigaction(SIGUSR1, &action_SIGUSR, NULL);
    sigaction(SIGUSR2, &action_SIGUSR, NULL);
}

void modify_SIGRT_behaviour(void (*receiver)(int, siginfo_t *, void *)) {
    sigset_t activated_signals;
    sigfillset(&activated_signals);
    sigdelset(&activated_signals, SIGRTMIN);
    sigdelset(&activated_signals, SIGRTMAX);

    sigprocmask(SIG_BLOCK, &activated_signals, NULL);

    struct sigaction action_SIGRT;
    action_SIGRT.sa_sigaction = receiver;
    action_SIGRT.sa_flags = SA_SIGINFO;
    sigemptyset(&action_SIGRT.sa_mask);

    sigaction(SIGRTMIN, &action_SIGRT, NULL);
    sigaction(SIGRTMAX, &action_SIGRT, NULL);
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}