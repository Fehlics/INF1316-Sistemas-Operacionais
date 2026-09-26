#include "trabalho.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
 * Envia a solicitacao ao KernelSim pelo pipe REAL de controle.
 * A escrita de uma struct pequena cabe em PIPE_BUF e, portanto, pedidos
 * vindos de aplicacoes diferentes nao se misturam dentro do pipe.
 *
 * IMPORTANTE: nesta etapa a syscall e apenas COMUNICADA, sem bloquear.
 * A conclusao da operacao, o retorno do RECV e o bloqueio virao depois.
 */
static int solicitar_syscall(int fd, int fd_resposta, int id, Operacao operacao, int pc, int *n) {
    PedidoSyscall pedido = {
        .id_aplicacao = id,
        .operacao = operacao,
        .pc = pc,
        .n = *n
    };

    ssize_t escritos;
    do {
        escritos = write(fd, &pedido, sizeof pedido);
    } while (escritos == -1 && errno == EINTR);

    if (escritos != (ssize_t)sizeof pedido) {
        perror("Application: write syscall");
        return -1;
    }

    printf("[A%d] Solicitou %s (PC=%d, N=%d)\n", id,
           operacao == ENVIAR ? "SEND" : "RECV", pc, *n);

    /* Espera a resposta exclusiva do KernelSim; ainda pode estar sob SIGSTOP. */
    RespostaSyscall resposta;
    size_t recebidos = 0;
    while (recebidos < sizeof resposta) {
        ssize_t lidos = read(fd_resposta, (char *)&resposta + recebidos,
                             sizeof resposta - recebidos);
        if (lidos == 0) return -1;
        if (lidos < 0) {
            if (errno == EINTR) continue;
            perror("Application: read resposta");
            return -1;
        }
        recebidos += (size_t)lidos;
    }
    if (resposta.id_aplicacao != id || resposta.operacao != operacao) {
        fprintf(stderr, "[A%d] Resposta invalida.\n", id);
        return -1;
    }
    if (operacao == RECEBER) *n = resposta.n;
    printf("[A%d] %s concluido (PC=%d, N=%d)\n",
           id, operacao == ENVIAR ? "SEND" : "RECV", pc, *n);
    return 0;
}

int main(int argc, char *argv[]) {
    /* O Simulador fornece o identificador e a ponta de escrita do pipe. */
    if (argc != 4) {
        fprintf(stderr, "Uso: Application ID FD_SYSCALL FD_RESPOSTA\n");
        return 1;
    }

    int id = atoi(argv[1]);
    int fd_syscall = atoi(argv[2]);
    int fd_resposta = atoi(argv[3]);
    if (id < 1 || id > QUANTIDADE_APLICACOES || fd_syscall < 0 || fd_resposta < 0) {
        fprintf(stderr, "Identificador ou descritor invalido.\n");
        return 1;
    }

    int pc = 1; /* Contador da propria aplicacao. */
    int n = 0;  /* Futuramente: contador recebido do parceiro. */
    int teste = getenv("TESTE_SYSCALL") != NULL;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < MAX_ITERACOES) {
        pc++;
        sleep(1);

        /*
         * Teste opcional: TESTE_SYSCALL=1 faz A1 solicitar SEND e A2
         * solicitar RECV no PC=2, facilitando a verificacao do canal.
         * No funcionamento normal, cada iteracao tem 15% de chance.
         */
        Operacao operacao = NENHUMA_OPERACAO;
        if (teste && pc == 2 && id == 1) {
            operacao = ENVIAR;
        } else if (teste && pc == 2 && id == 2) {
            operacao = RECEBER;
        } else if (rand() % 100 < 15) {
            operacao = (rand() % 2 == 0) ? ENVIAR : RECEBER;
        }

        if (operacao != NENHUMA_OPERACAO &&
            solicitar_syscall(fd_syscall, fd_resposta, id, operacao, pc, &n) == -1) {
            close(fd_syscall);
            close(fd_resposta);
            return 1;
        }

        if (pc % 100 == 0) {
            printf("[A%d] PC=%d N=%d\n", id, pc, n);
        }
    }

    printf("[A%d] Finalizada.\n", id);
    /* TODO: comunicar termino e aguardar conclusao das syscalls futuras. */
    close(fd_syscall);
    close(fd_resposta);
    return 0;
}
