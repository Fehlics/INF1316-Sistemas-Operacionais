/*
2) Mensagem do Dia
l Faça um programa que:
l Leia uma mensagem do dia do stdin (ou arquivo)
l Crie uma memória compartilhada com a chave
8752
l Salve a mensagem na memória
l Faça um outro programa “cliente” que utilize
a mesma chave (8752) e exiba a mensagem
do dia para o usuário
*/

//Aluno: Gabriel Félix de Farias Bereicoa - 2510619

/*
Compilação e Execução:
    gcc ex2Servidor.c -o ex2S
    ./ex2S

    (entrada da mensagem no teclado)
    
    gcc ex2Cliente.c -o ex2C
    ./ex2C
*/

#include <sys/shm.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

int main() {
    key_t chave = 8752;

    int shmid = shmget(chave, 256, 0);

    char *mensagem = (char*) shmat(shmid, NULL, 0);

    printf("Mensagem do Dia lida: %s\n", mensagem);

    shmdt(mensagem);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}