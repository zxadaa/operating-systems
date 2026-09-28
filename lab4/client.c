#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "common.h"

int client_qid = -1;
pid_t child_pid = -1;

void cleanup(int sig) {
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        wait(NULL);
    }
    if (client_qid != -1) {
        msgctl(client_qid, IPC_RMID, NULL); 
    }
    printf("\nzakończono działanie klienta\n");
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    //uzyskanie dostępu do kolejki serwera
    key_t server_key = ftok(getenv("HOME"), PROJ_ID);
    int server_qid = msgget(server_key, 0);
    if (server_qid == -1) {
        perror("błąd łączenia z serwerem");
        exit(1);
    }

    //tworzenie unikalnego klucza i kolejki komunikatów
    key_t client_key = ftok(getenv("HOME"), getpid());
    client_qid = msgget(client_key, IPC_CREAT | 0666);

    chat_msg_t msg;
    size_t msg_size = sizeof(chat_msg_t) - sizeof(long);

    //wysłanie komunikatu INIT do serwera
    msg.mtype = MSG_INIT;
    msg.client_queue_key = client_key;
    msgsnd(server_qid, &msg, msg_size, 0);

    //oczekiwanie na przydzielenie ID przez serwer na kolejce
    msgrcv(client_qid, &msg, msg_size, MSG_INIT, 0);
    int my_id = msg.client_id;
    printf("udana rejestracja, przydzielone ID: %d\n", my_id);

    //utworzenie drugiego procesu
    child_pid = fork();
    
    if (child_pid < 0) {
        perror("błąd fork");
        cleanup(0);
    }

    if (child_pid == 0) {
        //PROCES POTOMNY - odbiera wiadomości od innych
        while (1) {
            if (msgrcv(client_qid, &msg, msg_size, MSG_TEXT, 0) > 0) {
                printf("\r[Wiadomość od klienta o ID %d]: %s", msg.client_id, msg.text);
            }
        }
    } else {
        //PROCES MACIERZYSTY - czyta ze stdin i wysyła do serwera
        while (1) {
            fgets(msg.text, MAX_MSG_LEN, stdin);
            msg.mtype = MSG_TEXT;
            msg.client_id = my_id;
            
            msgsnd(server_qid, &msg, msg_size, 0);
        }
    }

    return 0;
}