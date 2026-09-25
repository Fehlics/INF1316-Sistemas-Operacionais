/* ex04c_shm_produto_pai.c  --  Lab 2 (exercicio 4: multiplicacao multi-processo)
 * Arquitetura:   P1 --(x, seq1)--> m1        m2 <--(y, seq2)-- P2        pai: res = x * y
 * Os filhos sao programas SEPARADOS (exec), entao NAO herdam o attach: recebem o shmid
 * como argumento de linha de comando e fazem shmat sozinhos.
 * Compile os dois:  make ex04c_shm_produto_pai ex04c_shm_produto_filho
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define RODADAS 5

typedef struct { volatile int valor; volatile int seq; } canal_t;

int main(void)
{
    int id1, id2;
    canal_t *m1, *m2;

    /* [resolvido] Crie DOIS segmentos (IPC_PRIVATE, sizeof(canal_t)) -> id1, id2; faca shmat em ambos */
    id1 = shmget(IPC_PRIVATE, sizeof(canal_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    id2 = shmget(IPC_PRIVATE, sizeof(canal_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (id1 < 0 || id2 < 0) { perror("shmget"); exit(1); }
    m1 = shmat(id1, NULL, 0);
    m2 = shmat(id2, NULL, 0);
    if (m1 == (void *)-1 || m2 == (void *)-1) { perror("shmat"); exit(1); }
    m1->valor = m1->seq = 0;
    m2->valor = m2->seq = 0;

    int ids[2] = {id1, id2};
    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            /* [resolvido] Converta ids[i] para string (snprintf) e faca */
            char idstr[16];
            snprintf(idstr, sizeof idstr, "%d", ids[i]);
            execl("./ex04c_shm_produto_filho", "filho", idstr, (char *)NULL);
            perror("execl");
            _exit(127);
        }
    }

    int vis1 = 0, vis2 = 0;          /* ultimo seq ja consumido de cada canal */
    int feitos = 0;
    while (feitos < RODADAS) {
        /* [resolvido] Espera ativa (com usleep(20000) para nao gastar CPU): so quando */
        if (m1->seq > vis1 && m2->seq > vis2) {
            int x = m1->valor, y = m2->valor;
            vis1 = m1->seq; vis2 = m2->seq;
            printf("rodada %d: %d * %d = %d\n", ++feitos, x, y, x * y);
        }
        usleep(20000);
    }

    wait(NULL); wait(NULL);
    shmdt(m1); shmdt(m2);
    shmctl(id1, IPC_RMID, NULL); shmctl(id2, IPC_RMID, NULL);
    return 0;
}
