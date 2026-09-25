/* ex04c_shm_produto_filho.c -- programa exec'ado pelo pai. Uso: ./ex04c_shm_produto_filho SHMID */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define RODADAS 5
typedef struct { volatile int valor; volatile int seq; } canal_t;

int main(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "uso: %s shmid\n", argv[0]); return 1; }

    int id;
    canal_t *c;
    /* [resolvido] 1) id = atoi(argv[1]); c = shmat(id, NULL, 0); trate erro. */
    id = atoi(argv[1]);
    c = shmat(id, NULL, 0);
    if (c == (void *)-1) { perror("shmat"); return 1; }

    srand((unsigned)(time(NULL) ^ (getpid() << 8)));
    for (int r = 0; r < RODADAS; r++) {
        /* [resolvido] 2) sleep(rand() % 3 + 1); grave c->valor = rand() % 9 + 1 E SO DEPOIS incremente */
        sleep((unsigned)(rand() % 3 + 1));
        c->valor = rand() % 9 + 1;
        c->seq++;
    }
    shmdt((void *)c);
    return 0;
}
