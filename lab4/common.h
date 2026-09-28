#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_MSG_LEN 256
#define MAX_CLIENTS 10
#define PROJ_ID 'S' 

#define MSG_INIT 1
#define MSG_TEXT 2

typedef struct {
    long mtype;             //typ komunikatu > 0
    int client_id;          //identyfikator klienta
    key_t client_queue_key; //klucz kolejki klienta
    char text[MAX_MSG_LEN]; //treść wiadomości
} chat_msg_t;

#endif