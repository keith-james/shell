#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "signals.h"

static volatile sig_atomic_t sigint_received = 0;

static void handle_sigint(int sig) {
    (void)sig;
    sigint_received = 1;
    write(STDOUT_FILENO, "\n", 1);
}

void init_signals(void) {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

int was_sigint_received(void) {
    return sigint_received;
}

void reset_sigint_flag(void) {
    sigint_received = 0;
}
