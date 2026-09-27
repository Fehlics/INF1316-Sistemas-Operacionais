#include "trabalho.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* CONTEXTO, SYSCALL e TERMINO compartilham a mesma mensagem de controle. */
static int enviar_evento(int fd, TipoEvento tipo, int id,
                         Operacao operacao, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = tipo;
    m.pedido.id_aplicacao = id;
    m.pedido.operacao = operacao;
    m.pedido.endereco = operacao == ENVIAR ? ENDERECO_PC :
                        operacao == RECEBER ? ENDERECO_N : SEM_ENDERECO;
    m.pedido.pc = pc;
    m.pedido.n = n;
    return enviar_dados(fd, &m, sizeof m);
}

static int solicitar_syscall(int controle, int fd_resposta, int id,
                            Operacao op, int *pc, int *n) {
    if (!enviar_evento(controle, EVENTO_SYSCALL, id, op, *pc, *n)) {
        perror("Application: envio da syscall");
        return 0;
    }
    printf("[A%d] Solicitou %s (PC=%d, N=%d)\n", id,
           op == ENVIAR ? "SEND" : "RECV", *pc, *n);
    RespostaSyscall r;
    if (!receber_dados(fd_resposta, &r, sizeof r) ||
        r.id_aplicacao != id || r.operacao != op) return 0;
    *pc = r.pc;
    *n = r.n;
    printf("[A%d] %s concluido (PC=%d, N=%d)\n", id,
           op == ENVIAR ? "SEND" : "RECV", *pc, *n);
    return 1;
}

static int avisar_termino(int controle, int resposta,
                         int id, int pc, int n) {
    if (!enviar_evento(controle, EVENTO_TERMINO, id,
                      NENHUMA_OPERACAO, pc, n)) {
        perror("Application: aviso de termino");
        return 0;
    }
    RespostaSyscall r;
    return receber_dados(resposta, &r, sizeof r) &&
           r.id_aplicacao == id && r.operacao == NENHUMA_OPERACAO &&
           r.pc == pc;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: Application ID FD_CONTROLE FD_RESPOSTA\n");
        return 1;
    }
    int id = atoi(argv[1]);
    int controle = atoi(argv[2]);
    int resposta = atoi(argv[3]);
    if (id < 1 || id > QUANTIDADE_APLICACOES ||
        controle < 0 || resposta < 0) return 1;

    int limite = MAX_ITERACOES;
    const char *teste_max = getenv("TESTE_MAX_ITERACOES");
    if (teste_max) {
        int n = atoi(teste_max);
        if (n >= 1 && n <= MAX_ITERACOES) limite = n;
    }
    int intervalo = intervalo_teste("TESTE_INTERVALO_US", 0, 1);
    int teste = getenv("TESTE_SYSCALL") != NULL;
    int sem_syscall = getenv("TESTE_SEM_SYSCALL") != NULL;
    int pc = 0, n = 0;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < limite) {
        pc++;
        if (!enviar_evento(controle, EVENTO_CONTEXTO, id,
                          NENHUMA_OPERACAO, pc, n)) goto erro;
        if (intervalo) usleep((useconds_t)intervalo);
        else sleep(1);

        Operacao op = NENHUMA_OPERACAO;
        if (!sem_syscall && teste && pc == 2 && id == 1) op = ENVIAR;
        else if (!sem_syscall && teste && pc == 2 && id == 2) op = RECEBER;
        else if (!sem_syscall && !teste && rand() % 100 < 15)
            op = rand() % 2 == 0 ? ENVIAR : RECEBER;

        if (op != NENHUMA_OPERACAO) {
            if (!solicitar_syscall(controle, resposta, id, op, &pc, &n) ||
                !enviar_evento(controle, EVENTO_CONTEXTO, id,
                              NENHUMA_OPERACAO, pc, n)) goto erro;
        }
        if (pc % 100 == 0) printf("[A%d] PC=%d N=%d\n", id, pc, n);
    }
    printf("[A%d] Finalizada (PC=%d, N=%d).\n", id, pc, n);
    if (!avisar_termino(controle, resposta, id, pc, n)) goto erro;
    printf("[A%d] Termino confirmado pelo KernelSim.\n", id);
    close(controle);
    close(resposta);
    return 0;

erro:
    close(controle);
    close(resposta);
    return 1;
}
