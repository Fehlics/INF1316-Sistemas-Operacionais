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
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct {
    int valor;
    int sequencia;
} BlocoDados;

int main(int argc, char *argv[]) {
    if (argc < 2) exit(1);

    int shm_id = atoi(argv[1]);
    BlocoDados *dados = (BlocoDados*) shmat(shm_id, NULL, 0);

    srand(time(NULL) ^ getpid());

    int seq = 1;
    for (int i = 0; i < 3; i++) {
        sleep(rand() % 3 + 1);
        dados->valor = rand() % 10 + 1;
        dados->sequencia = seq;
        seq++;
    }

    shmdt(dados);
    return 0;
}