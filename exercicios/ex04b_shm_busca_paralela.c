/* ex04b_shm_busca_paralela.c  --  Lab 2 (exercicio 3: busca paralela em vetor)
 * Vetor de NELEM ints em memoria compartilhada, dividido em NPROC fatias. Cada filho
 * procura a chave na SUA fatia e grava a posicao achada (ou -1) em res[i] (2o segmento).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define NELEM 40
#define NPROC 4
#define CHAVE_BUSCADA 42

static int *cria_shm(size_t n, int *id)      /* ja pronta: veja o ex04 */
{
    *id = shmget(IPC_PRIVATE, n * sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (*id < 0) { perror("shmget"); exit(1); }
    int *p = shmat(*id, NULL, 0);
    if (p == (void *)-1) { perror("shmat"); exit(1); }
    return p;
}

int main(void)
{
    int idv, idr;
    int *v   = cria_shm(NELEM, &idv);
    int *res = cria_shm(NPROC, &idr);

    for (int i = 0; i < NELEM; i++) v[i] = (i * 37 + 11) % 100;   /* desordenado */
    v[27] = CHAVE_BUSCADA;
    for (int i = 0; i < NPROC; i++) res[i] = -1;

    int fatia = NELEM / NPROC;
    for (int i = 0; i < NPROC; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            // TODO: FILHO i: percorra v[i*fatia .. (i+1)*fatia - 1]. Se v[k] == CHAVE_BUSCADA,
            //       grave res[i] = k (posicao GLOBAL) e pare. Depois exit(0).
            // >>> escreva seu codigo aqui <<<
        }
    }
    for (int i = 0; i < NPROC; i++) wait(NULL);

    // TODO: PAI: percorra res[] e informe qual processo achou a chave e em que posicao
    //       (ou "nao encontrada").
    // >>> escreva seu codigo aqui <<<

    shmdt(v); shmdt(res);
    shmctl(idv, IPC_RMID, NULL); shmctl(idr, IPC_RMID, NULL);
    return 0;
}
