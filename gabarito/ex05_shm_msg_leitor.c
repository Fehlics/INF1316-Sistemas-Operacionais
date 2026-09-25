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
    /* [resolvido] 1) id = shmget(CHAVE, TAM, 0600)  -- SEM IPC_CREAT: se o escritor nao rodou, deve falhar. */
    int id = shmget(CHAVE, TAM, 0600);
    if (id < 0) { perror("shmget (o escritor ja rodou?)"); return 1; }
    const char *msg = shmat(id, NULL, SHM_RDONLY);
    if (msg == (void *)-1) { perror("shmat"); return 1; }
    printf("Mensagem do dia: %s\n", msg);
    shmdt(msg);

    /* [resolvido] 5) Se argc > 1 e argv[1] for "-r": remova o segmento com shmctl(id, IPC_RMID, NULL). */
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        shmctl(id, IPC_RMID, NULL);
        puts("(segmento removido)");
    }
    return 0;
}
