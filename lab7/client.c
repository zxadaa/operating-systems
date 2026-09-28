#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char const *argv[]) {
    //sprawdzenie poprawności argumentów
    if (argc != 4) {
        printf("użycie: %s <IP> <PORT> <LICZBA>\n", argv[0]);
        printf("przykład: %s 127.0.0.1 9000 5\n", argv[0]);
        return -1;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    //pobranie argumentów
    const char* ip_adres = argv[1];
    int port = atoi(argv[2]);
    const char* liczba_str = argv[3];

    //tworzenie gniazda
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n błąd tworzenia gniazda \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    //konwersja adresów IPv4 i IPv6 z tekstu na postać binarną
    if (inet_pton(AF_INET, ip_adres, &serv_addr.sin_addr) <= 0) {
        printf("\n nieprawidłowy adres\n");
        return -1;
    }

    //nawiązanie połączenia
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n połączenie nie powiodło się \n");
        return -1;
    }

    //przygotowanie i wysłanie wiadomości
    char message[256];
    snprintf(message, sizeof(message), "ZADANIE %s", liczba_str);
    send(sock, message, strlen(message), 0);

    //odbiór i wyświetlenie odpowiedzi
    read(sock, buffer, 1024);
    printf("%s\n", buffer);

    //zamknięcie połączenia
    close(sock);
    return 0;
}