#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "signals.h"

#ifdef DYNAMIC_LOAD
#include <dlfcn.h>
#endif

volatile sig_atomic_t is_configured = 0;

void sig_unblock() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void usr2_handler(int sig, siginfo_t *info, void *context) {
    int action = info->si_value.sival_int;
    printf("[CHILD] odebrano SIGUSR2 od rodzica, kod akcji: %d\n", action);

#ifdef DYNAMIC_LOAD
    void *handle = dlopen("./libsignals.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "błąd dlopen: %s\n", dlerror());
        exit(1);
    }

    void (*func)() = NULL;

    switch(action) {
        case 1: func = dlsym(handle, "sig_default"); break;
        case 2: func = dlsym(handle, "sig_mask"); break;
        case 3: func = dlsym(handle, "sig_ignore"); break;
        case 4: func = dlsym(handle, "sig_handle"); break;
        default: printf("[CHILD] otrzymano nieznaną akcję\n");
    }

    if (func) {
        func(); 
    }
    
#else
    switch(action) {
        case 1: sig_default(); break;
        case 2: sig_mask(); break;
        case 3: sig_ignore(); break;
        case 4: sig_handle(); break;
        default: printf("[CHILD] otrzymano nieznaną akcję\n");
    }
#endif

    is_configured = 1;
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = usr2_handler;
    sa.sa_flags = SA_SIGINFO; 
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    printf("[CHILD] czekanie na konfigurację sygnałem SIGUSR2 od procesu main\n");
    while (!is_configured) { pause(); }

    for (int i = 1; i <= 20; i++) {
        printf("%d\n", i);
        if (i == 5 || i == 15) {
            printf("wysyłam sygnał USR1\n");
            raise(SIGUSR1);
        }
        if (i == 10) {
            sigset_t pending_set;
            sigpending(&pending_set); 
            if (sigismember(&pending_set, SIGUSR1)) {
                printf("odblokowuję USR1\n");
                sig_unblock();
            }
        }
        sleep(1);
    }
    printf("pętla została wykonana w całości\n");
    return 0;
}