#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "użycie: %s <M>\n", argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);
    pid_t pid = getpid();

    for (int i = 0; i < M; i++) {
        printf("potomek (PID: %d)\n", pid);
        fflush(stdout); 
        
        usleep(0.25); 
    }

    return 0;
}