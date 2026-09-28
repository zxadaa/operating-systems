#include <stdio.h>
#include <signal.h>

void sig_default() {
    printf("wywołano funkcję 'sig_default()'\n");
    signal(SIGUSR1, SIG_DFL);
}