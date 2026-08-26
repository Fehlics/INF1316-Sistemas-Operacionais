/*
4) Multiplicação multi-processo
Faça um programa que:
lTenha um processo pai que abre dois blocos de memória
compartilhada, m1 e m2.
lcria dois processos filho (use exec), P1 e P2: estes
também fazem attach em m1 ou m2 respectivamente
lCada um dá um sleep() randômico e escreve um valor int
na área compartilhada dele, e avisa o processo pai que um
novo valor foi gerado, escrevendo tb um nr de sequencia
lO pai fica em loop verificando se houve um novo valor.
Apenas quando ambos P1 e P2 geraram um novo valor, o
pai imprime o produto dos valores gerados por P1 e P2
*/

//Aluno: Gabriel Félix de Farias Bereicoa - 2510619

/*
Compilação e Execução:
    gcc ex4Filho.c -o filho
    gcc ex4Pai.c -o pai
    ./pai
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

typedef struct {
    int valor;
    int sequencia;
} BlocoDados;

int main() {
    int shm_m1 = shmget(IPC_PRIVATE, sizeof(BlocoDados), IPC_CREAT | S_IRUSR | S_IWUSR);
    int shm_m2 = shmget(IPC_PRIVATE, sizeof(BlocoDados), IPC_CREAT | S_IRUSR | S_IWUSR);

    BlocoDados *m1 = (BlocoDados*) shmat(shm_m1, NULL, 0);
    BlocoDados *m2 = (BlocoDados*) shmat(shm_m2, NULL, 0);

    m1->sequencia = 0;
    m2->sequencia = 0;

    char str_shm1[20], str_shm2[20];
    sprintf(str_shm1, "%d", shm_m1);
    sprintf(str_shm2, "%d", shm_m2);

    if (fork() == 0) {
        execl("./filho", "filho", str_shm1, NULL);
    }

    if (fork() == 0) {
        execl("./filho", "filho", str_shm2, NULL);
    }

    int ultima_seq_m1 = 0;
    int ultima_seq_m2 = 0;

    while (ultima_seq_m1 < 3 || ultima_seq_m2 < 3) {
        if (m1->sequencia > ultima_seq_m1 && m2->sequencia > ultima_seq_m2) {
            ultima_seq_m1 = m1->sequencia;
            ultima_seq_m2 = m2->sequencia;
            int produto = m1->valor * m2->valor;
            printf("[PAI] Novo par recebido (Seq %d): %d * %d = %d\n",
                   ultima_seq_m1, m1->valor, m2->valor, produto);
        }
        usleep(100000);
    }

    wait(NULL);
    wait(NULL);

    shmdt(m1);
    shmdt(m2);
    shmctl(shm_m1, IPC_RMID, NULL);
    shmctl(shm_m2, IPC_RMID, NULL);

    return 0;
}