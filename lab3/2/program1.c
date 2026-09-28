#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_REQ "/tmp/calka_req"
#define FIFO_RES "/tmp/calka_res"

int main() {
    //tworzenie potoków nazwanych (uprawnienia 0666 pozwalają na odczyt/zapis)
    mkfifo(FIFO_REQ, 0666);
    mkfifo(FIFO_RES, 0666);

    double params[3]; //poczatek_a, koniec_b, krok_dx]
    
    //pobieranie danych ze standardowego wejścia
    printf("--- PROGRAM 1 ---\n");
    printf("podaj poczatek przedzialu (a): ");
    scanf("%lf", &params[0]);
    printf("podaj koniec przedzialu (b): ");
    scanf("%lf", &params[1]);
    printf("podaj szerokosc prostokata (dx): ");
    scanf("%lf", &params[2]);

    printf("oczekiwanie na podlaczenie Programu 2...\n");

    //otwarcie potoku zapytania do zapisu, zablokowane dopóki Program 2 go nie otworzy
    int fd_req = open(FIFO_REQ, O_WRONLY);
    if (fd_req == -1) {
        perror("blad otwarcia potoku REQ");
        exit(1);
    }

    //wysłanie tablicy parametrów i zamknięcie zapisu
    write(fd_req, params, sizeof(params));
    close(fd_req);
    printf("wyslano parametry do obliczen\n");

    //otwarcie potoku wyniku do odczytu, zablkowoane dopóki Program 2 nie wyśle wyniku
    int fd_res = open(FIFO_RES, O_RDONLY);
    if (fd_res == -1) {
        perror("blad otwarcia potoku RES");
        exit(1);
    }

    //odbiór wyniku
    double result;
    read(fd_res, &result, sizeof(result));
    close(fd_res);

    printf("\notrzymany wynik calki: %.15f\n", result);

    unlink(FIFO_REQ);
    unlink(FIFO_RES);

    return 0;
}