/*
3) Busca paralela em vetor
l Faça um programa paralelo (com pelo menos
4 processos) para localizar uma chave em
um vetor.
l Crie uma memória compartilhada com dados
numéricos inteiros e desordenados e a divida
pelo número de processos
l Cada processo deve procurar o dado na sua área
de memória e informar a posição onde o dado foi
localizado.
*/

//Aluno: Gabriel Félix de Farias Bereicoa - 2510619

/*
Compilação e Execução:
    gcc ex3.c -o ex3
    ./ex3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main() {
    int num_processos = 4;
    int tamanho_vetor = 16;
    int chave_busca = 42;

    size_t tamanho_bytes = tamanho_vetor * sizeof(int);
    int shm_id = shmget(IPC_PRIVATE, tamanho_bytes, IPC_CREAT | S_IRUSR | S_IWUSR);
    int *vetor = (int*) shmat(shm_id, NULL, 0);

    for (int i = 0; i < tamanho_vetor; i++) {
        vetor[i] = (i + 1) * 3;
    }
    vetor[7] = chave_busca;

    int bloco = tamanho_vetor / num_processos;

    for (int i = 0; i < num_processos; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int inicio = i * bloco;
            int fim = inicio + bloco;

            for (int j = inicio; j < fim; j++) {
                if (vetor[j] == chave_busca) {
                    printf("[Processo %d] Chave %d encontrada na posição %d!\n", i, chave_busca, j);
                }
            }

            shmdt(vetor);
            exit(0);
        }
    }

    for (int i = 0; i < num_processos; i++) {
        wait(NULL);
    }

    shmdt(vetor);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}