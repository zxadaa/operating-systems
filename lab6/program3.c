#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <stdatomic.h>

//globalne bufory
frame_buffer_t left_cam_buf;
frame_buffer_t right_cam_buf;
frame_buffer_t sync_cam_buf;
state_buffer_t robot_state_buf;

//zmienne atomowe do statystyk
atomic_int stat_left_frames = 0;
atomic_int stat_right_frames = 0;
atomic_int stat_synced_frames = 0;
atomic_int stat_robot_states = 0;

//obsluga zamykania
void handle_sigint(int sig) {
    (void)sig;
    printf("\n[SYSTEM] otrzymano sygnal ctrl c zamykanie i generowanie raportu\n");
    keep_running = 0;

    pthread_cond_broadcast(&left_cam_buf.not_full);
    pthread_cond_broadcast(&left_cam_buf.not_empty);
    pthread_cond_broadcast(&right_cam_buf.not_full);
    pthread_cond_broadcast(&right_cam_buf.not_empty);
    pthread_cond_broadcast(&sync_cam_buf.not_full);
    pthread_cond_broadcast(&sync_cam_buf.not_empty);
    pthread_cond_broadcast(&robot_state_buf.not_full);
    pthread_cond_broadcast(&robot_state_buf.not_empty);
}

//watek watchdoga
void* watchdog_thread(void* arg) {
    int prev_l = 0, prev_r = 0, prev_s = 0;
    
    while (keep_running) {
        sleep_ms(1000); 
        if (!keep_running) break;

        int curr_l = atomic_load(&stat_left_frames);
        int curr_r = atomic_load(&stat_right_frames);
        int curr_s = atomic_load(&stat_robot_states);

        int diff_l = curr_l - prev_l;
        int diff_r = curr_r - prev_r;
        int diff_s = curr_s - prev_s;

        if (diff_l > 0 && diff_l < 20) printf("[WATCHDOG] LEFT CAMERA SLOW (%d hz)\n", diff_l);
        if (diff_r > 0 && diff_r < 20) printf("[WATCHDOG] RIGHT CAMERA SLOW (%d hz)\n", diff_r);
        if (diff_s > 0 && diff_s < 80) printf("[WATCHDOG] ROBOT STATE SLOW (%d hz)\n", diff_s);

        prev_l = curr_l; prev_r = curr_r; prev_s = curr_s;
    }
    return NULL;
}

//raport koncowy
void generate_report() {
    FILE *f = fopen("report.txt", "w");
    if (f) {
        fprintf(f, "--- RAPORT KONCOWY ---\n");
        fprintf(f, "ramki lewej kamery: %d\n", atomic_load(&stat_left_frames));
        fprintf(f, "ramki prawej kamery: %d\n", atomic_load(&stat_right_frames));
        fprintf(f, "zsynchronizowane pary: %d\n", atomic_load(&stat_synced_frames));
        fprintf(f, "wygenerowane stany robota: %d\n", atomic_load(&stat_robot_states));
        fclose(f);
        printf("[SYSTEM] wygenerowano plik report.txt\n");
    }
}

//watki kamer
void* left_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        frame_t frame = {id++, get_current_time_ms()};
        frame_buffer_push(&left_cam_buf, frame);
        atomic_fetch_add(&stat_left_frames, 1);
        sleep_ms(40); 
    }
    return NULL;
}

void* right_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        frame_t frame = {id++, get_current_time_ms()};
        frame_buffer_push(&right_cam_buf, frame);
        atomic_fetch_add(&stat_right_frames, 1);
        sleep_ms(40); 
    }
    return NULL;
}

void* sync_thread(void* arg) {
    while (keep_running) {
        frame_t l_frame = frame_buffer_pop(&left_cam_buf);
        frame_t r_frame = frame_buffer_pop(&right_cam_buf);
        
        if (!keep_running) break;

        long long diff = l_frame.timestamp_ms - r_frame.timestamp_ms;
        if (abs((int)diff) < 20) {
            frame_buffer_push(&sync_cam_buf, l_frame);
            atomic_fetch_add(&stat_synced_frames, 1);
        }
    }
    return NULL;
}

void* image_writer_thread(void* arg) {
    while (keep_running) {
        frame_t frame = frame_buffer_pop(&sync_cam_buf);
        if (!keep_running) break;
        sleep_ms(100); 
    }
    return NULL;
}

//watek stanu robota rtos
void* robot_state_thread(void* arg) {
    float angle = 0.0;
    while (keep_running) {
        robot_state_t state = {1.5 + cos(angle), 2.5 + sin(angle), angle, get_current_time_ms()};
        angle += 0.05;
        if(angle > M_PI) angle -= 2 * M_PI;

        state_buffer_push(&robot_state_buf, state);
        atomic_fetch_add(&stat_robot_states, 1);
        sleep_ms(10); 
    }
    return NULL;
}

void* logger_thread(void* arg) {
    FILE* file = fopen("robot_log.txt", "w");
    while (keep_running) {
        robot_state_t state = state_buffer_pop(&robot_state_buf);
        if (!keep_running) break;

        if (file) {
            fprintf(file, "Czas: %lld Pos: %.2f %.2f Theta: %.2f\n", state.timestamp_ms, state.x, state.y, state.theta);
            fflush(file);
        }
        sleep_ms(100); 
    }
    if (file) fclose(file);
    return NULL;
}

//glowna funkcja
int main() {
    signal(SIGINT, handle_sigint);
    printf("[SYSTEM] start systemu");

    frame_buffer_init(&left_cam_buf);
    frame_buffer_init(&right_cam_buf);
    frame_buffer_init(&sync_cam_buf);
    state_buffer_init(&robot_state_buf);

    pthread_t t_lcam, t_rcam, t_sync, t_writer, t_robot, t_logger, t_watchdog;

    pthread_create(&t_lcam, NULL, left_camera_thread, NULL);
    pthread_create(&t_rcam, NULL, right_camera_thread, NULL);
    pthread_create(&t_sync, NULL, sync_thread, NULL);
    pthread_create(&t_writer, NULL, image_writer_thread, NULL);
    pthread_create(&t_robot, NULL, robot_state_thread, NULL);
    pthread_create(&t_logger, NULL, logger_thread, NULL);
    pthread_create(&t_watchdog, NULL, watchdog_thread, NULL);

    //ustawienie priorytetu
    struct sched_param param;
    param.sched_priority = 90; 
    if (pthread_setschedparam(t_robot, SCHED_FIFO, &param) != 0) {
        printf("[SYSTEM] brak uprawnien do sched_fifo wymagany root watek robota dziala normalnie\n");
    } else {
        printf("[SYSTEM] pomyslnie ustawiono priorytet rtos dla watku robota\n");
    }

    while (keep_running) {
        sleep_ms(1000); 
    }

    pthread_join(t_lcam, NULL);
    pthread_join(t_rcam, NULL);
    pthread_join(t_sync, NULL);
    pthread_join(t_writer, NULL);
    pthread_join(t_robot, NULL);
    pthread_join(t_logger, NULL);
    pthread_join(t_watchdog, NULL);

    generate_report(); 
    return 0;
}