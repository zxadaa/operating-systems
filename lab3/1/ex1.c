#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


double f(double x) {
    return 4.0 / (x * x + 1.0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "użycie: %s <szerokość_prostokąta> <max_liczba_procesów_n>\n", argv[0]);
        return 1;
    }

    double dx = atof(argv[1]);
    int n = atoi(argv[2]);

    if (dx <= 0.0 || n <= 0) {
        fprintf(stderr, "błąd: parametry muszą być wartościami dodatnimi\n");
        return 1;
    }

    long long N = (long long)(1.0 / dx);

    printf("liczba prostokątów (N): %lld\n\n", N);

    for (int k = 1; k <= n; ++k) {
        int pipes[k][2];
        pid_t pids[k];

        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        //tworzenie potokow
        for (int i = 0; i < k; ++i) {
            if (pipe(pipes[i]) == -1) {
                perror("błąd tworzenia potoku");
                exit(1);
            }
        }

        //tworzenie procesów potomnych
        for (int i = 0; i < k; ++i) {
            pids[i] = fork();
            if (pids[i] == -1) {
                perror("błąd fork");
                exit(1);
            }

            if (pids[i] == 0) { //PROCES POTOMNY
                for (int j = 0; j < k; ++j) {
                    close(pipes[j][0]);//zamykanie koncow odczytu
                    if (j != i) {
                        close(pipes[j][1]); //zamykanie końcow zapisu dla potoków innych niż własny
                    }
                }

                //określenie zakresu indeksów prostokątów dla tego procesu
                long long start_idx = i * N / k;
                long long end_idx = (i == k - 1) ? N : (i + 1) * N / k;

                double partial_sum = 0.0;
                
                //obliczanie częściowej całki
                for (long long j = start_idx; j < end_idx; ++j) {
                    double x = j * dx;
                    partial_sum += f(x) * dx;
                }

                //zapisanie wyniku do własnego potoku
                if (write(pipes[i][1], &partial_sum, sizeof(double)) == -1) {
                    perror("błąd zapisu do potoku");
                }
                
                close(pipes[i][1]); //zamknięcie potoku po wysłaniu danych
                exit(0);           
            }
        }

        //PROCES MACIERZYSTY
        double total_sum = 0.0;

        //zamknięcie końcówek do pisania we wszystkich potokach
        for (int i = 0; i < k; ++i) {
            close(pipes[i][1]);
        }

        //oczekiwanie na zakończenie wszystkich procesów potomnych
        for (int i = 0; i < k; ++i) {
            waitpid(pids[i], NULL, 0);
        }

        //odczytywanie wyników i sumowanie
        for (int i = 0; i < k; ++i) {
            double partial_sum;
            if (read(pipes[i][0], &partial_sum, sizeof(double)) > 0) {
                total_sum += partial_sum;
            }
            close(pipes[i][0]); //zamknięcie potoku po odczycie
        }

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                         (end_time.tv_nsec - start_time.tv_nsec) / 1e9;


        printf("k = %2d | Wynik: %.15f | Czas wykonania: %.6f s\n", k, total_sum, elapsed);
    }

    return 0;
}