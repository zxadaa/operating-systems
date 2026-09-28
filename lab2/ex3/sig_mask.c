#include <stdio.h>
#include <signal.h>

void sig_mask() {
    printf("wywołano funkcję 'sig_mask()'\n");
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);
}
