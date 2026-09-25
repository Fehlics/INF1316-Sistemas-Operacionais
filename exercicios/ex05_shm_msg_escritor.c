/* ex05_shm_msg_escritor.c  --  Lab 2 (exercicio 2: "Mensagem do Dia")
 * Diferente do ex03: aqui os processos NAO sao parentes, entao nao da pra usar
 * IPC_PRIVATE. Os dois programas combinam uma CHAVE fixa (8752).
 *   ./ex05_shm_msg_escritor   -> grava a mensagem
 *   ./ex05_shm_msg_leitor     -> le e mostra (use -r para remover o segmento)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CHAVE 8752
#define TAM   256

int main(void)
{
    char linha[TAM];
    printf("Mensagem do dia: ");
    if (!fgets(linha, sizeof linha, stdin)) return 1;
    linha[strcspn(linha, "\n")] = '\0';

    // TODO: 1) id = shmget(CHAVE, TAM, IPC_CREAT | 0600)   (sem IPC_EXCL: pode reexecutar)
    //       2) msg = shmat(id, NULL, 0)  (trate erros)
    //       3) copie 'linha' para msg com strncpy (garanta o '\0' final)  4) shmdt(msg)
    // >>> escreva seu codigo aqui <<<

    printf("Mensagem gravada na chave %d. (ipcs -m para ver o segmento)\n", CHAVE);
    return 0;
}
