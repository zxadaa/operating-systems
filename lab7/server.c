#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000
#define BUFFER_SIZE 2048

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int LICZNIK_ZAPYTAN = 0;
    char buffer[BUFFER_SIZE] = {0};

    //tworzenie gniazda
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("błąd tworzenia gniazda");
        exit(EXIT_FAILURE);
    }

    //ustawienie opcji gniazda
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //nasłuch na wszystkich interfejsach
    address.sin_port = htons(PORT);

    //bindowanie gniazda do portu
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("błąd bindowania");
        exit(EXIT_FAILURE);
    }

    //czekiwanie na połączenia
    if (listen(server_fd, 3) < 0) {
        perror("błąd nasłuchiwania");
        exit(EXIT_FAILURE);
    }

    printf("serwer uruchomiony. nasłuchiwanie na porcie %d\n", PORT);

    while (1) {
        //przyjęcie połączenia
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("błąd akceptacji połączenia");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        //odczytanie przesłanych danych
        read(new_socket, buffer, BUFFER_SIZE - 1);

        //obsługa zapytań HTTP (GET lub POST)
        if (strncmp(buffer, "GET", 3) == 0 || strncmp(buffer, "POST", 4) == 0) {
            LICZNIK_ZAPYTAN++;
            
            char body[256];
            int body_len = snprintf(body, sizeof(body), "liczba pobrań strony: %d", LICZNIK_ZAPYTAN);

            char header[512];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Server: serwer SO\r\n"
                     "Content-Type: text/plain; charset=utf-8\r\n"
                     "Connection: close\r\n"
                     "Cache-Control: no-store\r\n"
                     "Content-Length: %d\r\n\r\n", body_len);

            //odesłanie nagłówka i treści
            write(new_socket, header, strlen(header));
            write(new_socket, body, body_len);
        }
        //obsługa zapytania testowego klienta
        else if (strncmp(buffer, "ZADANIE", 7) == 0) {
            int wartosc = 0;
            sscanf(buffer, "ZADANIE %d", &wartosc);
            LICZNIK_ZAPYTAN += wartosc;

            char body[256];
            int body_len = snprintf(body, sizeof(body), "liczba pobrań strony: %d", LICZNIK_ZAPYTAN);
            
            //odesłanie samej treści
            write(new_socket, body, body_len);
        }

        //zamknięcie połączenia
        close(new_socket);
    }

    return 0;
}