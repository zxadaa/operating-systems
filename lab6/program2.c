#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

//globalne bufory
frame_buffer_t left_cam_buf;
frame_buffer_t right_cam_buf;
frame_buffer_t sync_cam_buf;
state_buffer_t robot_state_buf;

//globalne statystyki
volatile int stat_total_frames = 0;
volatile int stat_total_robot_states = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

//obsluga zamykania
void handle_sigint(int sig) {
    (void)sig;
    printf("\n[SYSTEM] otrzymano sygnal ctrl^c - zamykanie\n");
    keep_running = 0;

    //budzenie wszystkich zablokowanych wątków dla każdego bufora
    pthread_cond_broadcast(&left_cam_buf.not_full);
    pthread_cond_broadcast(&left_cam_buf.not_empty);
    pthread_cond_broadcast(&right_cam_buf.not_full);
    pthread_cond_broadcast(&right_cam_buf.not_empty);
    pthread_cond_broadcast(&sync_cam_buf.not_full);
    pthread_cond_broadcast(&sync_cam_buf.not_empty);
    pthread_cond_broadcast(&robot_state_buf.not_full);
    pthread_cond_broadcast(&robot_state_buf.not_empty);
}

//watki kamer
void* left_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        frame_t frame = {id++, get_current_time_ms()};
        frame_buffer_push(&left_cam_buf, frame);
        
        pthread_mutex_lock(&stats_mutex);
        stat_total_frames++;
        pthread_mutex_unlock(&stats_mutex);
        
        sleep_ms(40); //25 hz
    }
    return NULL;
}

void* right_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        frame_t frame = {id++, get_current_time_ms()};
        frame_buffer_push(&right_cam_buf, frame);
        sleep_ms(40); //25 hz
    }
    return NULL;
}

//watek synchronizacji obrazow
void* sync_thread(void* arg) {
    while (keep_running) {
        frame_t l_frame = frame_buffer_pop(&left_cam_buf);
        frame_t r_frame = frame_buffer_pop(&right_cam_buf);
        
        if (!keep_running) break;

        long long diff = l_frame.timestamp_ms - r_frame.timestamp_ms;
        if (abs((int)diff) < 20) {
            frame_buffer_push(&sync_cam_buf, l_frame);
        }
    }
    return NULL;
}

//watek zapisu obrazow
void* image_writer_thread(void* arg) {
    while (keep_running) {
        frame_t frame = frame_buffer_pop(&sync_cam_buf);
        if (!keep_running) break;
        
        sleep_ms(100); 
    }
    return NULL;
}

//watek stanu robota
void* robot_state_thread(void* arg) {
    float angle = 0.0;
    while (keep_running) {
        robot_state_t state = {
            1.5 + cos(angle), 
            2.5 + sin(angle), 
            angle, 
            get_current_time_ms()
        };
        angle += 0.05;
        if(angle > M_PI) angle -= 2 * M_PI;

        state_buffer_push(&robot_state_buf, state);
        
        pthread_mutex_lock(&stats_mutex);
        stat_total_robot_states++;
        pthread_mutex_unlock(&stats_mutex);

        sleep_ms(10); //100 hz
    }
    return NULL;
}

//watek loggera
void* logger_thread(void* arg) {
    FILE* file = fopen("robot_log.txt", "w");
    while (keep_running) {
        robot_state_t state = state_buffer_pop(&robot_state_buf);
        if (!keep_running) break;

        if (file) {
            fprintf(file, "czas: %lld position: %.2f %.2f theta: %.2f\n", 
                    state.timestamp_ms, state.x, state.y, state.theta);
            fflush(file);
        }
        sleep_ms(100); //10 hz
    }
    if (file) fclose(file);
    return NULL;
}

//glowna funkcja
int main() {
    //rejestracja obslugi sygnalu
    signal(SIGINT, handle_sigint);

    printf("[SYSTEM] start systemu wcisnij ctrl c aby wyjsc\n");

    frame_buffer_init(&left_cam_buf);
    frame_buffer_init(&right_cam_buf);
    frame_buffer_init(&sync_cam_buf);
    state_buffer_init(&robot_state_buf);

    pthread_t t_lcam, t_rcam, t_sync, t_writer, t_robot, t_logger;

    //tworzenie wątków
    pthread_create(&t_lcam, NULL, left_camera_thread, NULL);
    pthread_create(&t_rcam, NULL, right_camera_thread, NULL);
    pthread_create(&t_sync, NULL, sync_thread, NULL);
    pthread_create(&t_writer, NULL, image_writer_thread, NULL);
    pthread_create(&t_robot, NULL, robot_state_thread, NULL);
    pthread_create(&t_logger, NULL, logger_thread, NULL);

    int last_frames = 0;
    int last_states = 0;

    //glowna petla programu zastepuje sleep i wyswietla statystyki
    while (keep_running) {
        sleep_ms(2000); //co 2 sekundy
        if (!keep_running) break;

        pthread_mutex_lock(&stats_mutex);
        int current_frames = stat_total_frames;
        int current_states = stat_total_robot_states;
        pthread_mutex_unlock(&stats_mutex);

        //obliczanie czestotliwosci
        float cam_hz = (current_frames - last_frames) / 2.0;
        float robot_hz = (current_states - last_states) / 2.0;

        printf("\n--- statystyki systemu ---\n");
        printf("wygenerowane ramki: %d czestotliwosc %.1f hz\n", current_frames, cam_hz);
        printf("dane robota: %d czestotliwosc %.1f hz\n", current_states, robot_hz);
        printf("--------------------------\n");

        last_frames = current_frames;
        last_states = current_states;
    }

    //dolaczanie watkow poprawnie
    pthread_join(t_lcam, NULL);
    pthread_join(t_rcam, NULL);
    pthread_join(t_sync, NULL);
    pthread_join(t_writer, NULL);
    pthread_join(t_robot, NULL);
    pthread_join(t_logger, NULL);

    printf("[SYSTEM] wszystkie watki zakonczone pomyslnie\n");
    return 0;
}