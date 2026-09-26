#include "trabalho.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Apenas a estrutura inicial do escalonador está implementada. */
static Processo processos[QUANTIDADE_APLICACOES];
static int atual = -1;

static void iniciar_processos(char *argumentos[]) {
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        processos[i].id = i + 1;
        processos[i].pid = (pid_t)atol(argumentos[i]);
        processos[i].estado = PRONTO;
        processos[i].operacao_pendente = NENHUMA_OPERACAO;
        /* Os demais campos iniciam em zero (variável static). */
    }
}

static void escalonar(void) {
    /* Interrompe a aplicação que estava usando a CPU simulada. */
    if (atual != -1 && processos[atual].estado == EXECUTANDO) {
        kill(processos[atual].pid, SIGSTOP);
        processos[atual].estado = PRONTO;
    }

    /* Round Robin simplificado: procura a próxima aplicação PRONTA. */
    for (int passo = 1; passo <= QUANTIDADE_APLICACOES; passo++) {
        int proximo = (atual + passo) % QUANTIDADE_APLICACOES;
        if (processos[proximo].estado == PRONTO) {
            atual = proximo;
            processos[atual].estado = EXECUTANDO;
            kill(processos[atual].pid, SIGCONT);
            printf("[Kernel] Executando A%d\n", processos[atual].id);
            return;
        }
    }

    atual = -1; /* Nenhuma aplicação pronta. */
}

static void tratar_irq(MensagemIRQ mensagem) {
    switch (mensagem.tipo) {
        case IRQ0:
            escalonar();
            break;
        case IRQ1:
            puts("[Kernel] IRQ1 (TODO: concluir primeiro recv pendente)");
            break;
        case IRQ2:
            puts("[Kernel] IRQ2 (TODO: concluir primeiro send pendente)");
            break;
        default:
            puts("[Kernel] IRQ desconhecida.");
    }
}

int main(int argc, char *argv[]) {
    if (argc != QUANTIDADE_APLICACOES + 2) {
        fprintf(stderr, "Uso: KernelSim FD_IRQ PID_A1 ... PID_A6\n");
        return 1;
    }

    setbuf(stdout, NULL);
    int fd_irq = atoi(argv[1]);
    iniciar_processos(&argv[2]);

    /* Inicia o Round Robin colocando A1 para executar. */
    escalonar();

    for (;;) {
        MensagemIRQ mensagem;
        ssize_t lidos = read(fd_irq, &mensagem, sizeof mensagem);
        if (lidos == 0) break; /* Controlador fechou o pipe. */
        if (lidos == -1) {
            if (errno == EINTR) continue;
            perror("read IRQ");
            break;
        }
        if (lidos == (ssize_t)sizeof mensagem) tratar_irq(mensagem);

        /* TODO: receber syscalls das aplicações e atualizar os PCBs. */
        /* TODO: filas READ/WRITE e seis buffers dos pipes simulados. */
        /* TODO: reconhecer aplicações TERMINADAS e imprimir seus estados. */
    }

    close(fd_irq);
    return 0;
}
