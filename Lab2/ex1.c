/*
1) Soma de matrizes
Faça um programa para somar matrizes de acordo
com o seguinte algoritmo
l O primeiro processo irá criar duas matrizes
preenchidas e uma terceira vazia em 3 áreas de
memória compartilhada.
l Para cada linha da matriz solução, o seu
programa deverá gerar um processo para o seu
cálculo.
OBS: implemente as matrizes como vetores de tamanho
(linha x coluna) e aloque a shared memory para os vetores
correspondentes, pois acessar os elementos (i,j) é
complexo. 
*/

//Aluno: Gabriel Félix de Farias Bereicoa - 2510619

/*
Compilação e Execução:
    gcc ex1.c -o ex1
    ./ex1
*/

#include <unistd.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/wait.h>

int main (){
    int linhas = 3;
    int colunas = 3;
    int tamanho_total = linhas * colunas;
    size_t tamanho_bytes = tamanho_total * sizeof(int);

    int shm_a = shmget(IPC_PRIVATE, tamanho_bytes, IPC_CREAT | S_IRUSR | S_IWUSR);
    int shm_b = shmget(IPC_PRIVATE, tamanho_bytes, IPC_CREAT | S_IRUSR | S_IWUSR);
    int shm_res = shmget(IPC_PRIVATE, tamanho_bytes, IPC_CREAT | S_IRUSR | S_IWUSR);

    int *A = (int*) shmat(shm_a, NULL, 0);
    int *B = (int*) shmat(shm_b, NULL, 0);
    int *R = (int*) shmat(shm_res, NULL, 0);

    for (int i = 0; i < tamanho_total; i++) {
        A[i] = i + 1;
        B[i] = (i + 1) * 2;
    }

    for (int i = 0; i < linhas; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            for (int j = 0; j < colunas; j++) {
                int idx = i * colunas + j;
                R[idx] = A[idx] + B[idx];
            }

            shmdt(A);
            shmdt(B);
            shmdt(R);
            exit(0);
        }
    }

    for (int i = 0; i < linhas; i++) {
        wait(NULL);
    }

    printf("Matriz Resultante:\n");
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            printf("%d ", R[i * colunas + j]);
        }
        printf("\n");
    }

    shmdt(A);
    shmdt(B);
    shmdt(R);
    shmctl(shm_a, IPC_RMID, NULL);
    shmctl(shm_b, IPC_RMID, NULL);
    shmctl(shm_res, IPC_RMID, NULL);

    return 0;
}