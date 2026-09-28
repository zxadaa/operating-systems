#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "common.h"

int server_qid = -1;
int client_queues[MAX_CLIENTS];
int client_count = 0;

//sygnał Ctrl+C
void cleanup(int sig) {
    printf("\nzamykanie serwera...\n");
    if (server_qid != -1) {
        msgctl(server_qid, IPC_RMID, NULL);
    }
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    //generowanie klucza
    key_t server_key = ftok(getenv("HOME"), PROJ_ID);
    
    //tworzenie kolejki komunikatów serwera
    server_qid = msgget(server_key, IPC_CREAT | 0666);
    if (server_qid == -1) {
        perror("błąd msgget serwera");
        exit(1);
    }

    printf("serwer uruchomiony, czekam na klientów...\n");
    chat_msg_t msg;
    size_t msg_size = sizeof(chat_msg_t) - sizeof(long);

    while (1) {
        //najstarsza wiadomość, blokowanie
        if (msgrcv(server_qid, &msg, msg_size, 0, 0) == -1) {
            perror("błąd msgrcv");
            continue;
        }

        if (msg.mtype == MSG_INIT) {
            if (client_count < MAX_CLIENTS) {
                //otwieranie prywatnej kolejki klienta
                int client_qid = msgget(msg.client_queue_key, 0);
                client_queues[client_count] = client_qid;

                //przypisanie identyfikatora
                msg.client_id = client_count;
                
                //wysłanie ID na prywatną kolejkę klienta
                msgsnd(client_qid, &msg, msg_size, 0);

                printf("nowy klient podłączony i zarejestrowany pod ID: %d\n", client_count);
                client_count++;
            } else {
                printf("odrzucono próbę połączenia - osiągnięto limit klientów\n");
            }
        } 
        else if (msg.mtype == MSG_TEXT) {
            printf("przekazywanie wiadomości od klienta o ID %d\n", msg.client_id);
            
            //przesyłanie wiadomości na kolejki wszystkich pozostałych klientów, blokowanie
            for (int i = 0; i < client_count; i++) {
                if (i != msg.client_id) {
                    msgsnd(client_queues[i], &msg, msg_size, 0);
                }
            }
        }
    }
    return 0;
}