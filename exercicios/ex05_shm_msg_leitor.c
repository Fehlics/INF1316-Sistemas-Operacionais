/* ex05_shm_msg_leitor.c -- par do ex05_shm_msg_escritor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CHAVE 8752
#define TAM   256

int main(int argc, char *argv[])
{
    // TODO: 1) id = shmget(CHAVE, TAM, 0600)  -- SEM IPC_CREAT: se o escritor nao rodou, deve falhar.
    //       2) msg = shmat(id, NULL, SHM_RDONLY)  -- somente leitura
    //       3) imprima a mensagem  4) shmdt(msg)
    // >>> escreva seu codigo aqui <<<

    // TODO: 5) Se argc > 1 e argv[1] for "-r": remova o segmento com shmctl(id, IPC_RMID, NULL).
    // >>> escreva seu codigo aqui <<<
    return 0;
}
