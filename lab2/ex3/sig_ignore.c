#include <stdio.h>
#include <signal.h>

void sig_ignore() {
    printf("wywołano funkcję 'sig_ignore()'\n");
    signal(SIGUSR1, SIG_IGN);
}