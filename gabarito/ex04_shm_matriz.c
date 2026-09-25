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
    /* [resolvido] shmget(IPC_PRIVATE, n * sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR) */
    *id = shmget(IPC_PRIVATE, n * sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (*id < 0) { perror("shmget"); exit(1); }
    int *p = shmat(*id, NULL, 0);
    if (p == (void *)-1) { perror("shmat"); exit(1); }
    return p;
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
        /* [resolvido] Crie o filho da linha i (fork). No filho: para cada coluna j faca */
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            for (int j = 0; j < COL; j++)
                C[i * COL + j] = A[i * COL + j] + B[i * COL + j];
            exit(0);
        }
    }

    /* [resolvido] Espere os L filhos (wait(NULL) L vezes). */
    for (int i = 0; i < L; i++) wait(NULL);

    imprime("A", A); imprime("B", B); imprime("C = A + B", C);

    /* [resolvido] Limpeza: shmdt nas 3 matrizes e shmctl(id, IPC_RMID, NULL) nos 3 ids. */
    shmdt(A); shmdt(B); shmdt(C);
    shmctl(ida, IPC_RMID, NULL); shmctl(idb, IPC_RMID, NULL); shmctl(idc, IPC_RMID, NULL);
    return 0;
}
