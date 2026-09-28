#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 16

//struktury danych
typedef struct {
    int frame_id;
    long long timestamp_ms;
} frame_t;

typedef struct {
    float x;
    float y;
    float theta;
    long long timestamp_ms;
} robot_state_t;

//bufory cykliczne fifo
typedef struct {
    frame_t data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} frame_buffer_t;

typedef struct {
    robot_state_t data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} state_buffer_t;

//deklaracje funkcji
long long get_current_time_ms();
void sleep_ms(int ms);

void frame_buffer_init(frame_buffer_t *buf);
void frame_buffer_push(frame_buffer_t *buf, frame_t frame);
frame_t frame_buffer_pop(frame_buffer_t *buf);

void state_buffer_init(state_buffer_t *buf);
void state_buffer_push(state_buffer_t *buf, robot_state_t state);
robot_state_t state_buffer_pop(state_buffer_t *buf);

//zmienna globalna kontrolujaca dzialanie systemu
extern volatile int keep_running;

#endif