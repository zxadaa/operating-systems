#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


void sig_default() {
    printf("wywołano funkcję 'sig_default()'\n");
    signal(SIGUSR1, SIG_DFL);
}

void sig_mask() {
    printf("wywołano funkcję 'sig_mask()'\n");
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void sig_ignore() {
    printf("wywołano funkcję 'sig_ignore()'\n");
    signal(SIGUSR1, SIG_IGN);
}

void handler(int sig) {
    printf("wywołano handler dla sygnału %d\n", sig);
}

void sig_handle() {
    printf("wywołano funkcję 'sig_handle()'\n");
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
}


void sig_unblock() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "default") == 0) {
        sig_default();
    } else if (strcmp(argv[1], "mask") == 0) {
        sig_mask();
    } else if (strcmp(argv[1], "ignore") == 0) {
        sig_ignore();
    } else if (strcmp(argv[1], "handle") == 0) {
        sig_handle();
    } else {
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }


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