#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXFILA 8
#define TOTAL_ELEMENTOS 64

int fila[MAXFILA];
int count = 0;
int in = 0;
int out = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pode_produzir = PTHREAD_COND_INITIALIZER;
pthread_cond_t pode_consumir = PTHREAD_COND_INITIALIZER;

void *Produtor(void *arg) {
    for (int i = 1; i <= TOTAL_ELEMENTOS; i++) {
        sleep(1);
        int dado = rand() % 100;

        pthread_mutex_lock(&mutex);

        while (count == MAXFILA) {
            pthread_cond_wait(&pode_produzir, &mutex);
        }

        fila[in] = dado;
        in = (in + 1) % MAXFILA;
        count++;
        printf("[Produtor] Produziu: %d | Fila: %d/%d\n", dado, count, MAXFILA);

        pthread_cond_signal(&pode_consumir);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *Consumidor(void *arg) {
    for (int i = 1; i <= TOTAL_ELEMENTOS; i++) {
        sleep(2);
        pthread_mutex_lock(&mutex);

        while (count == 0)
            pthread_cond_wait(&pode_consumir, &mutex);
        int dado = fila[out];
        out = (out + 1) % MAXFILA;
        count--;
        printf("[Consumidor] Consumiu: %d | Fila: %d/%d\n", dado, count, MAXFILA);
        pthread_cond_signal(&pode_produzir);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t prod, cons;
    pthread_create(&prod, NULL, Produtor, NULL);
    pthread_create(&cons, NULL, Consumidor, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    printf("Processamento concluido!\n");
    return 0;
}