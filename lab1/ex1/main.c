#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define M 5

int globalVar = 0; 

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "użycie: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);

    for (int i = 0; i < N; i++) {
        pid_t pid = vfork(); 

        if (pid == 0) {
            globalVar++;
            
            for (int j = 0; j < M; j++) {
                printf("potomek (PID: %d)\n", getpid());
                fflush(stdout); 
                sleep(0.25); 
            }
            exit(0); 
        }
    }

    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    printf("rodzic (PID: %d) globalvar = %d\n", getpid(), globalVar);

    return 0;
}