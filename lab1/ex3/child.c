#include "definitions.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "użycie: %s <M>\n", argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);
    pid_t pid = getpid();

    FILE *file = fopen(OUTPUT_FILE, "a");
    if (file == NULL) {
        perror("błąd otwarcia pliku");
        return 1;
    }

    //pobieranie deskryptora pliku
    int fd = fileno(file);

    //założenie blokady na wyłączność
    if (flock(fd, LOCK_EX) == -1) {
        perror("błąd zakładania blokady");
        fclose(file);
        return 1;
    }

    char buffer[256];

    for (int i = 0; i < M; i++) {

        int len = snprintf(buffer, sizeof(buffer), "Potomek (PID: %d)\n", pid);
        fwrite(buffer, sizeof(char), len, file);
        fflush(file); 
        
        usleep(0.25); 
    }

    if (flock(fd, LOCK_UN) == -1) {
        perror("błąd zdejmowania blokady");
    }

    fclose(file);
    return 0;
}