#include <stdio.h>
#include <signal.h>

void handler(int sig) {
    printf("wywołano handler dla sygnału %d\n", sig);
}

void sig_handle() {
    printf("wywołano funkcję 'sig_handle()'\n");
    signal(SIGUSR1, handler);
}