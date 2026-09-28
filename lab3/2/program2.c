#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_REQ "/tmp/calka_req"
#define FIFO_RES "/tmp/calka_res"


double f(double x) {
    return 4.0 / (x * x + 1.0);
}

int main() {
    printf("--- PROGRAM 2 ---\n");
    printf("uruchomiono tryb liczenia, oczekuje na dane z Programu 1...\n");

    //otwarcie potoku zapytania do odczytu
    int fd_req = open(FIFO_REQ, O_RDONLY);
    if (fd_req == -1) {
        perror("blad otwarcia potoku REQ");
        exit(1);
    }

    //odbiór parametrów
    double params[3];
    read(fd_req, params, sizeof(params));
    close(fd_req);

    double a = params[0];
    double b = params[1];
    double dx = params[2];

    printf("ddebrano: przedzial [%.2f, %.2f], dx = %f\n", a, b, dx);
    printf("trwaja obliczenia...\n");

    //obliczanie całki metodą prostokątów
    long long N = (long long)((b - a) / dx);
    double total_sum = 0.0;
    
    for (long long i = 0; i < N; ++i) {
        double x = a + i * dx;
        total_sum += f(x) * dx;
    }

    //otwarcie potoku wyniku do zapisu
    int fd_res = open(FIFO_RES, O_WRONLY);
    if (fd_res == -1) {
        perror("blad otwarcia potoku RES");
        exit(1);
    }

    write(fd_res, &total_sum, sizeof(total_sum));
    close(fd_res);

    printf("wynik odeslany\n");

    return 0;
}