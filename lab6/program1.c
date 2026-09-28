
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//lewa kamera
frame_t shared_left_frame;
pthread_mutex_t mutex_left = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_left;

//prawa kamera
frame_t shared_right_frame;
pthread_mutex_t mutex_right = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_right;

//zsynchronizowane pary
frame_t shared_sync_frame;
pthread_mutex_t mutex_sync = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_sync;

//stan robota
robot_state_t shared_state;
pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_state;


//wątki kamer
void* left_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        pthread_mutex_lock(&mutex_left);
        shared_left_frame.frame_id = id++;
        shared_left_frame.timestamp_ms = get_current_time_ms();
        pthread_mutex_unlock(&mutex_left);
        
        sem_post(&sem_left); 
        sleep_ms(40);        //25 Hz = 40 ms
    }
    return NULL;
}

void* right_camera_thread(void* arg) {
    int id = 1;
    while (keep_running) {
        pthread_mutex_lock(&mutex_right);
        shared_right_frame.frame_id = id++;
        shared_right_frame.timestamp_ms = get_current_time_ms();
        pthread_mutex_unlock(&mutex_right);
        
        sem_post(&sem_right);
        sleep_ms(40);        //25 Hz
    }
    return NULL;
}

//wątek synchronizacji obrazów
void* sync_thread(void* arg) {
    frame_t l_frame, r_frame;
    while (keep_running) {
        //czeka na dane z obu kamer
        sem_wait(&sem_left);
        sem_wait(&sem_right);

        //odczyt lewej ramki
        pthread_mutex_lock(&mutex_left);
        l_frame = shared_left_frame;
        pthread_mutex_unlock(&mutex_left);

        //odczyt prawej ramki
        pthread_mutex_lock(&mutex_right);
        r_frame = shared_right_frame;
        pthread_mutex_unlock(&mutex_right);

        //porównanie znaczników czasu
        long long diff = l_frame.timestamp_ms - r_frame.timestamp_ms;
        if (abs((int)diff) < 20) {
            //przekazanie do wątku zapisu
            pthread_mutex_lock(&mutex_sync);
            shared_sync_frame = l_frame;
            pthread_mutex_unlock(&mutex_sync);
            
            sem_post(&sem_sync);
        }
    }
    return NULL;
}

//wątek zapisu obrazów
void* image_writer_thread(void* arg) {
    frame_t frame_to_save;
    while (keep_running) {
        sem_wait(&sem_sync); //czeka na zsynchronizowaną ramkę

        pthread_mutex_lock(&mutex_sync);
        frame_to_save = shared_sync_frame;
        pthread_mutex_unlock(&mutex_sync);

        printf("[WRITER] zapisano obrazy: left_%04d.jpg, right_%04d.jpg\n", 
               frame_to_save.frame_id, frame_to_save.frame_id);
               
        sleep_ms(100); //10 Hz
    }
    return NULL;
}

//wątek stanu robota
void* robot_state_thread(void* arg) {
    float current_x = 0.0;
    float current_y = 0.0;
    float current_theta = 0.0;
    
    //prędkość liniowa i kątowa
    float speed = 0.05; 
    float turn_rate = 0.02;

    while (keep_running) {
        current_x += speed * cos(current_theta);
        current_y += speed * sin(current_theta);
        current_theta += turn_rate;

        //utrzymanie kąta w zakresie od -PI do PI
        if (current_theta > M_PI) current_theta -= 2 * M_PI;

        //zapis do współdzielonej pamięci
        pthread_mutex_lock(&mutex_state);
        shared_state.x = current_x;
        shared_state.y = current_y;
        shared_state.theta = current_theta;
        shared_state.timestamp_ms = get_current_time_ms();
        pthread_mutex_unlock(&mutex_state);

        //powiadomienie loggera
        sem_post(&sem_state);
        sleep_ms(10); // 100 Hz
    }
    return NULL;
}

//wątek loggera
void* logger_thread(void* arg) {
    FILE* file = fopen("robot_log.txt", "w");
    if (!file) {
        perror("błąd otwarcia pliku logu");
        return NULL;
    }

    robot_state_t state_to_log;
    while (keep_running) {
        sem_wait(&sem_state); //czeka na nowy stan

        pthread_mutex_lock(&mutex_state);
        state_to_log = shared_state;
        pthread_mutex_unlock(&mutex_state);

        fprintf(file, "czas: %lld, position: (%.2f, %.2f), theta: %.2f\n",
                state_to_log.timestamp_ms, state_to_log.x, state_to_log.y, state_to_log.theta);
        fflush(file);
        
        sleep_ms(100); //10 Hz
    }
    
    fclose(file);
    return NULL;
}

//główna funkcja
int main() {
    printf("[SYSTEM] start systemu.\n");

    //inicjalizacja semaforów
    sem_init(&sem_left, 0, 0);
    sem_init(&sem_right, 0, 0);
    sem_init(&sem_sync, 0, 0);
    sem_init(&sem_state, 0, 0);

    pthread_t t_lcam, t_rcam, t_sync, t_writer, t_robot, t_logger;

    //tworzenie wątków
    pthread_create(&t_lcam, NULL, left_camera_thread, NULL);
    pthread_create(&t_rcam, NULL, right_camera_thread, NULL);
    pthread_create(&t_sync, NULL, sync_thread, NULL);
    pthread_create(&t_writer, NULL, image_writer_thread, NULL);
    pthread_create(&t_robot, NULL, robot_state_thread, NULL);
    pthread_create(&t_logger, NULL, logger_thread, NULL);

  
    for (int i = 0; i < 20; i++) {
        sleep_ms(1000);
    }

    printf("\n[SYSTEM] trwa zamykanie.\n");
    keep_running = 0;

    //wymuszenie odblokowania semaforów, gdyby wątki na nich wisiały
    sem_post(&sem_left);
    sem_post(&sem_right);
    sem_post(&sem_sync);
    sem_post(&sem_state);

    //oczekiwanie na zakończenie wątków
    pthread_join(t_lcam, NULL);
    pthread_join(t_rcam, NULL);
    pthread_join(t_sync, NULL);
    pthread_join(t_writer, NULL);
    pthread_join(t_robot, NULL);
    pthread_join(t_logger, NULL);

    //niszczenie zasobów
    sem_destroy(&sem_left);
    sem_destroy(&sem_right);
    sem_destroy(&sem_sync);
    sem_destroy(&sem_state);

    printf("[SYSTEM] wszystkie wątki zakończone pomyślnie.\n");
    return 0;
}