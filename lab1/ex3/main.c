#include "definitions.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "użycie: %s <N> <M>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    char *M_str = argv[2]; 

    remove(OUTPUT_FILE);

    // tworzenie N procesów potomnych
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("błąd fork");
            return 1;
        } else if (pid == 0) {
            // proces potomny 
            execl("./child", "./child", M_str, NULL);
            perror("błąd execl");
            exit(1);
        }
    }

    // proces macierzysty
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    printf("rodzic  (PID: %d)\n", getpid());

    return 0;
}