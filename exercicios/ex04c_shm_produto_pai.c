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

    // TODO: Crie DOIS segmentos (IPC_PRIVATE, sizeof(canal_t)) -> id1, id2; faca shmat em ambos
    //       (m1, m2) e inicialize valor = 0 e seq = 0 nos dois.
    // >>> escreva seu codigo aqui <<<

    int ids[2] = {id1, id2};
    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            // TODO: Converta ids[i] para string (snprintf) e faca
            //       execl("./ex04c_shm_produto_filho", "filho", idstr, (char *)NULL); trate a falha.
            // >>> escreva seu codigo aqui <<<
        }
    }

    int vis1 = 0, vis2 = 0;          /* ultimo seq ja consumido de cada canal */
    int feitos = 0;
    while (feitos < RODADAS) {
        // TODO: Espera ativa (com usleep(20000) para nao gastar CPU): so quando
        //       m1->seq > vis1 E m2->seq > vis2, imprima "x * y = produto", atualize vis1/vis2
        //       e incremente 'feitos'.
        // >>> escreva seu codigo aqui <<<
    }

    wait(NULL); wait(NULL);
    shmdt(m1); shmdt(m2);
    shmctl(id1, IPC_RMID, NULL); shmctl(id2, IPC_RMID, NULL);
    return 0;
}
