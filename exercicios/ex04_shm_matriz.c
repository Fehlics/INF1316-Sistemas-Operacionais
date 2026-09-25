/* ex04_shm_matriz.c  --  Lab 2 (exercicio 1: soma de matrizes)
 * C = A + B. Tres matrizes L x COL em memoria compartilhada, guardadas como VETORES
 * de L*COL ints (elemento (i,j) esta em [i*COL + j]).
 * Um processo filho por linha da matriz resultado.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define L   3
#define COL 3

/* Cria um segmento com n ints, devolve o ponteiro e o id (via *id). */
static int *cria_shm(size_t n, int *id)
{
    // TODO: shmget(IPC_PRIVATE, n * sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)
    //       + shmat; trate os dois erros; guarde o id em *id e devolva o ponteiro.
    // >>> escreva seu codigo aqui <<<
}

static void imprime(const char *nome, const int *m)
{
    printf("%s =\n", nome);
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < COL; j++) printf("%4d", m[i * COL + j]);
        putchar('\n');
    }
}

int main(void)
{
    int ida, idb, idc;
    int *A = cria_shm(L * COL, &ida);
    int *B = cria_shm(L * COL, &idb);
    int *C = cria_shm(L * COL, &idc);

    for (int i = 0; i < L; i++)
        for (int j = 0; j < COL; j++) {
            A[i * COL + j] = i * COL + j + 1;        /* 1..9 */
            B[i * COL + j] = (i == j) ? 10 : 0;      /* 10 na diagonal */
            C[i * COL + j] = 0;
        }

    for (int i = 0; i < L; i++) {
        // TODO: Crie o filho da linha i (fork). No filho: para cada coluna j faca
        //       C[i*COL+j] = A[i*COL+j] + B[i*COL+j]; depois exit(0).
        //       No pai: apenas continue o laco (crie TODOS os filhos antes de esperar).
        // >>> escreva seu codigo aqui <<<
    }

    // TODO: Espere os L filhos (wait(NULL) L vezes).
    // >>> escreva seu codigo aqui <<<

    imprime("A", A); imprime("B", B); imprime("C = A + B", C);

    // TODO: Limpeza: shmdt nas 3 matrizes e shmctl(id, IPC_RMID, NULL) nos 3 ids.
    // >>> escreva seu codigo aqui <<<
    return 0;
}
