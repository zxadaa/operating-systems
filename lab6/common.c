#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include <time.h>

volatile int keep_running = 1;

long long get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)(ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    
#ifdef __APPLE__
    nanosleep(&ts, NULL);
#else
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
#endif
}

//implementacja bufora ramek
void frame_buffer_init(frame_buffer_t *buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
}

void frame_buffer_push(frame_buffer_t *buf, frame_t frame) {
    pthread_mutex_lock(&buf->mutex);
    
    while (buf->count == BUFFER_SIZE && keep_running) {
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    }
    
    if (!keep_running) {
        pthread_mutex_unlock(&buf->mutex);
        return;
    }
    
    buf->data[buf->head] = frame;
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count++;
    
    pthread_cond_signal(&buf->not_empty); 
    pthread_mutex_unlock(&buf->mutex);
}

frame_t frame_buffer_pop(frame_buffer_t *buf) {
    frame_t frame = {0, 0};
    pthread_mutex_lock(&buf->mutex);
    
    while (buf->count == 0 && keep_running) {
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    }
    
    if (!keep_running && buf->count == 0) {
        pthread_mutex_unlock(&buf->mutex);
        return frame;
    }
    
    frame = buf->data[buf->tail];
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    buf->count--;
    
    pthread_cond_signal(&buf->not_full); 
    pthread_mutex_unlock(&buf->mutex);
    return frame;
}

//implementacja bufora stanu
void state_buffer_init(state_buffer_t *buf) {
    buf->head = 0; buf->tail = 0; buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
}

void state_buffer_push(state_buffer_t *buf, robot_state_t state) {
    pthread_mutex_lock(&buf->mutex);
    while (buf->count == BUFFER_SIZE && keep_running) {
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    }
    if (!keep_running) { pthread_mutex_unlock(&buf->mutex); return; }
    buf->data[buf->head] = state;
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count++;
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->mutex);
}

robot_state_t state_buffer_pop(state_buffer_t *buf) {
    robot_state_t state = {0, 0, 0, 0};
    pthread_mutex_lock(&buf->mutex);
    while (buf->count == 0 && keep_running) {
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    }
    if (!keep_running && buf->count == 0) { pthread_mutex_unlock(&buf->mutex); return state; }
    state = buf->data[buf->tail];
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    buf->count--;
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->mutex);
    return state;
}