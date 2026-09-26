#include "trabalho.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
 * Ponto de extensão: futuramente, esta função enviará PedidoSyscall
 * para o KernelSim, aguardará o desbloqueio e receberá o resultado.
 */
static void solicitar_syscall(int id, Operacao operacao, int pc, int *n) {
    (void)id;
    (void)operacao;
    (void)pc;
    (void)n;
    /* TODO: enviar pedido real e aguardar o KernelSim. */
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: App ID (1 a 6)\n");
        return 1;
    }

    int id = atoi(argv[1]);
    if (id < 1 || id > QUANTIDADE_APLICACOES) {
        fprintf(stderr, "ID de aplicacao invalido.\n");
        return 1;
    }

    int pc = 1;   /* Contador próprio da aplicação. */
    int n = 0;    /* Último contador recebido do parceiro. */
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < MAX_ITERACOES) {
        pc++;
        sleep(1);

        /* Cerca de 15% de chance de solicitar uma syscall. */
        if (rand() % 100 < 15) {
            Operacao operacao = (rand() % 2 == 0) ? ENVIAR : RECEBER;
            solicitar_syscall(id, operacao, pc, &n);
        }

        if (pc % 100 == 0) {
            printf("[A%d] PC=%d N=%d\n", id, pc, n);
        }
    }

    printf("[A%d] Finalizada.\n", id);
    /* TODO: comunicar término ao KernelSim. */
    return 0;
}
