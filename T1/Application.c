#include "trabalho.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Envia o pedido pelo canal de controle e aguarda a resposta individual. */
static int solicitar_syscall(int fd_controle, int fd_resposta, int id,
                            Operacao operacao, int pc, int *n) {
    MensagemControle mensagem = {0};
    mensagem.tipo = EVENTO_SYSCALL;
    mensagem.pedido.id_aplicacao = id;
    mensagem.pedido.operacao = operacao;
    mensagem.pedido.pc = pc;
    mensagem.pedido.n = *n;

    ssize_t escritos;
    do {
        escritos = write(fd_controle, &mensagem, sizeof mensagem);
    } while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof mensagem) {
        perror("Application: envio da syscall");
        return 0;
    }
    printf("[A%d] Solicitou %s (PC=%d, N=%d)\n", id,
           operacao == ENVIAR ? "SEND" : "RECV", pc, *n);

    RespostaSyscall resposta;
    size_t total = 0;
    while (total < sizeof resposta) {
        ssize_t lidos = read(fd_resposta, (char *)&resposta + total,
                             sizeof resposta - total);
        if (lidos == 0) return 0;
        if (lidos < 0) {
            if (errno == EINTR) continue;
            perror("Application: resposta");
            return 0;
        }
        total += (size_t)lidos;
    }
    if (resposta.id_aplicacao != id || resposta.operacao != operacao) return 0;
    if (operacao == RECEBER) *n = resposta.n;
    printf("[A%d] %s concluido (PC=%d, N=%d)\n", id,
           operacao == ENVIAR ? "SEND" : "RECV", pc, *n);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: Application ID FD_CONTROLE FD_RESPOSTA\n");
        return 1;
    }
    int id = atoi(argv[1]);
    int fd_controle = atoi(argv[2]);
    int fd_resposta = atoi(argv[3]);
    if (id < 1 || id > QUANTIDADE_APLICACOES ||
        fd_controle < 0 || fd_resposta < 0) return 1;
    int pc = 1, n = 0;
    int teste = getenv("TESTE_SYSCALL") != NULL;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < MAX_ITERACOES) {
        pc++;
        sleep(1);
        Operacao operacao = NENHUMA_OPERACAO;
        if (teste && pc == 2 && id == 1) operacao = ENVIAR;
        else if (teste && pc == 2 && id == 2) operacao = RECEBER;
        else if (!teste && rand() % 100 < 15)
            operacao = rand() % 2 == 0 ? ENVIAR : RECEBER;

        if (operacao != NENHUMA_OPERACAO &&
            !solicitar_syscall(fd_controle, fd_resposta, id,
                              operacao, pc, &n)) {
            close(fd_controle);
            close(fd_resposta);
            return 1;
        }
        if (pc % 100 == 0) printf("[A%d] PC=%d N=%d\n", id, pc, n);
    }
    printf("[A%d] Finalizada.\n", id);
    /* Proxima etapa: avisar explicitamente o termino ao KernelSim. */
    close(fd_controle);
    close(fd_resposta);
    return 0;
}
