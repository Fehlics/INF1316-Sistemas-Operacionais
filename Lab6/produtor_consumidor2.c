#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXFILA 8
#define TOTAL_ELEMENTOS 64
#define NUM_PRODUTORES 2
#define NUM_CONSUMIDORES 2
#define ITENS_POR_PRODUTOR (TOTAL_ELEMENTOS / NUM_PRODUTORES)
#define ITENS_POR_CONSUMIDOR (TOTAL_ELEMENTOS / NUM_CONSUMIDORES)

int fila[MAXFILA];
int count = 0;
int in = 0;
int out = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pode_produzir = PTHREAD_COND_INITIALIZER;
pthread_cond_t pode_consumir = PTHREAD_COND_INITIALIZER;

void *Produtor(void *arg) {
    int id = *((int *)arg);

    for (int i = 1; i <= ITENS_POR_PRODUTOR; i++) {
        sleep(1); 
        int dado = rand() % 100;
        pthread_mutex_lock(&mutex);

        while (count == MAXFILA) {
            pthread_cond_wait(&pode_produzir, &mutex);
        }
        fila[in] = dado;
        in = (in + 1) % MAXFILA;
        count++;
        printf("[Produtor %d] Produziu: %d | Fila: %d/%d\n", id, dado, count, MAXFILA);

        pthread_cond_broadcast(&pode_consumir);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *Consumidor(void *arg) {
    int id = *((int *)arg);

    for (int i = 1; i <= ITENS_POR_CONSUMIDOR; i++) {
        sleep(2); 
        pthread_mutex_lock(&mutex);

        while (count == 0) {
            pthread_cond_wait(&pode_consumir, &mutex);
        }
        int dado = fila[out];
        out = (out + 1) % MAXFILA;
        count--;
        printf("[Consumidor %d] Consumiu: %d | Fila: %d/%d\n", id, dado, count, MAXFILA);

        pthread_cond_broadcast(&pode_produzir);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t prod[NUM_PRODUTORES];
    pthread_t cons[NUM_CONSUMIDORES];
    int id_prod[NUM_PRODUTORES];
    int id_cons[NUM_CONSUMIDORES];

    for (int i = 0; i < NUM_PRODUTORES; i++) {
        id_prod[i] = i + 1;
        pthread_create(&prod[i], NULL, Produtor, &id_prod[i]);
    }
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        id_cons[i] = i + 1;
        pthread_create(&cons[i], NULL, Consumidor, &id_cons[i]);
    }
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_join(prod[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        pthread_join(cons[i], NULL);
    }
    printf("Processamento concluido!\n");
    return 0;
}