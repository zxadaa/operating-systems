#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    int action_val = 0;
    if (strcmp(argv[1], "default") == 0) action_val = 1;
    else if (strcmp(argv[1], "mask") == 0) action_val = 2;
    else if (strcmp(argv[1], "ignore") == 0) action_val = 3;
    else if (strcmp(argv[1], "handle") == 0) action_val = 4;
    else {
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("błąd fork");
        return 1;
    }

    if (pid == 0) {
        execl("./child", "./child", NULL);
        
        perror("błąd execl"); 
        exit(1);

    } else {
        sleep(1);

        union sigval value;
        value.sival_int = action_val;

        printf("[MAIN] wysyłano sygnał SIGUSR2 do potomka (PID: %d) z kodem akcji: %d\n", pid, action_val);
        
        if (sigqueue(pid, SIGUSR2, value) == -1) {
            perror("błąd sigqueue");
        }

        wait(NULL);
        printf("[MAIN] potomek zakończył działanie\n");
    }

    return 0;
}