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
    // TODO: 1) id = atoi(argv[1]); c = shmat(id, NULL, 0); trate erro.
    // >>> escreva seu codigo aqui <<<

    srand((unsigned)(time(NULL) ^ (getpid() << 8)));
    for (int r = 0; r < RODADAS; r++) {
        // TODO: 2) sleep(rand() % 3 + 1); grave c->valor = rand() % 9 + 1 E SO DEPOIS incremente
        //       c->seq (o pai usa seq como "aviso de valor novo": o valor tem que estar pronto antes!).
        // >>> escreva seu codigo aqui <<<
    }
    shmdt((void *)c);
    return 0;
}
