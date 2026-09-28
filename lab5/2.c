#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define N 2           //liczba producentów
#define M 3           //liczba konsumentów
#define K 5           //rozmiar bufora
#define STR_LEN 11    
#define ITERATIONS 3  //liczba zadań genrowana przez każdego producenta

#define SHM_NAME "/shm_buffer"
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

//struktura pamięci współdzielonej
typedef struct {
    //NORMAL
    char buffer_normal[K][STR_LEN];
    int head_normal;
    int tail_normal;
    int count_normal;

    //PRIORITY
    char buffer_priority[K][STR_LEN];
    int head_priority;
    int tail_priority;
    int count_priority;
} SharedData;

//funkcja do generowania losowego łańcucha znaków
void generate_random_string(char *str, int length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < length; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length] = '\0';
}

int main() {
    //inicjalizacja pamięci współdzielonej
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, sizeof(SharedData));
    
    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    shared->head_normal = 0;
    shared->tail_normal = 0;
    shared->count_normal = 0;
    
    shared->head_priority = 0;
    shared->tail_priority = 0;
    shared->count_priority = 0;

    //inicjalizacja semaforówi
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);

    sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT, 0666, K);     //liczba pustych miejsc w buforze
    sem_t *full = sem_open(SEM_FULL, O_CREAT, 0666, 0);       //liczba elementów gotowych do pobrania

    //tworzenie procesów producentów
    for (int i = 0; i < N; i++) {
        if (fork() == 0) {
            srand(time(NULL) ^ (getpid() << 16)); //unikalny seed dla każdego procesu
            
            for (int j = 0; j < ITERATIONS; j++) {
                char task[STR_LEN];
                generate_random_string(task, STR_LEN - 1);
                
                //losowanie czy zadanie ma status PRIORITY
                int is_priority = (rand() % 100) < 30;

                sem_wait(empty); //czekanie na puste miejsce
                sem_wait(mutex); //wejście do sekcji krytycznej

                //zapis do bufora
                if (is_priority) {
                    strcpy(shared->buffer_priority[shared->tail_priority], task);
                    printf("[Producent %d] Wygenerowano zadanie PRIORITY: %s, zapisano pod indeks %d\n", getpid(), task, shared->tail_priority);
                    shared->tail_priority = (shared->tail_priority + 1) % K;
                    shared->count_priority++;
                } else {
                    strcpy(shared->buffer_normal[shared->tail_normal], task);
                    printf("[Producent %d] Wygenerowano zadanie NORMAL: %s, zapisano pod indeks %d\n", getpid(), task, shared->tail_normal);
                    shared->tail_normal = (shared->tail_normal + 1) % K;
                    shared->count_normal++;
                }

                sem_post(mutex); //wyjście z sekcji krytycznej
                sem_post(full);  //sygnalizacja, że dodano nowy element
                
                usleep(300000);
            }
            exit(EXIT_SUCCESS);
        }
    }

    //tworzenie procesów konsumentów
    for (int i = 0; i < M; i++) {
        if (fork() == 0) {
            int tasks_to_consume = (N * ITERATIONS) / M + ((i < (N * ITERATIONS) % M) ? 1 : 0);
            
            for (int j = 0; j < tasks_to_consume; j++) {
                char local_task[STR_LEN];
                int read_index = 0;
                int is_priority_task = 0;

                sem_wait(full);  //czekanie na element w buforze
                sem_wait(mutex); //wejście do sekcji krytycznej

                //odczyt z bufora
                if (shared->count_priority > 0) {
                    strcpy(local_task, shared->buffer_priority[shared->head_priority]);
                    read_index = shared->head_priority;
                    shared->head_priority = (shared->head_priority + 1) % K;
                    shared->count_priority--;
                    is_priority_task = 1;
                } else {
                    strcpy(local_task, shared->buffer_normal[shared->head_normal]);
                    read_index = shared->head_normal;
                    shared->head_normal = (shared->head_normal + 1) % K;
                    shared->count_normal--;
                    is_priority_task = 0;
                }

                sem_post(mutex); //wyjście z sekcji krytycznej
                sem_post(empty); //sygnalizacja, że zwolniono miejsce

                if (is_priority_task) {
                    printf("[Konsument %d] Czyta PRIORITY z %d: ", getpid(), read_index);
                } else {
                    printf("[Konsument %d] Czyta NORMAL z %d: ", getpid(), read_index);
                }

                for (int k = 0; k < STR_LEN - 1; k++) {
                    putchar(local_task[k]);
                    fflush(stdout);
                    usleep(300000);  //opóźnienie 0.3s
                }
                printf("\n");
            }
            exit(EXIT_SUCCESS);
        }
    }

    //oczekiwanie na zakończenie wszystkich procesów potomnych
    for (int i = 0; i < N + M; i++) {
        wait(NULL);
    }
    printf("\nwszystkie procesy zakonczyly dzialanie\n");

    munmap(shared, sizeof(SharedData));
    shm_unlink(SHM_NAME);
    
    sem_close(mutex);
    sem_close(empty);
    sem_close(full);
    
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);

    return 0;
}