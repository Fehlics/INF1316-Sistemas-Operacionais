#include "trabalho.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Processo processos[QUANTIDADE_APLICACOES];
static int atual = -1;

static void iniciar_processos(char *argumentos[]) {
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        processos[i].id = i + 1;
        processos[i].pid = (pid_t)atol(argumentos[i]);
        processos[i].estado = PRONTO;
        processos[i].operacao_pendente = NENHUMA_OPERACAO;
    }
}

/* Escalonador inicial: um processo de cada vez, seguindo Round Robin. */
static void escalonar(void) {
    if (atual != -1 && processos[atual].estado == EXECUTANDO) {
        kill(processos[atual].pid, SIGSTOP);
        processos[atual].estado = PRONTO;
    }

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

    atual = -1;
}

static void tratar_irq(MensagemIRQ mensagem) {
    switch (mensagem.tipo) {
        case IRQ0:
            escalonar();
            break;
        case IRQ1:
            puts("[Kernel] IRQ1 (TODO: concluir primeiro RECV bloqueado)");
            break;
        case IRQ2:
            puts("[Kernel] IRQ2 (TODO: concluir primeiro SEND bloqueado)");
            break;
        default:
            puts("[Kernel] IRQ desconhecida.");
    }
}

/*
 * Recebe e identifica pedidos de syscall.
 * Nesta etapa apenas registra os valores recebidos e imprime o pedido:
 * NAO bloqueia a aplicacao, NAO coloca em fila e NAO altera N.
 */
static void tratar_syscall(PedidoSyscall pedido) {
    if (pedido.id_aplicacao < 1 ||
        pedido.id_aplicacao > QUANTIDADE_APLICACOES ||
        (pedido.operacao != ENVIAR && pedido.operacao != RECEBER)) {
        puts("[Kernel] Pedido de syscall invalido.");
        return;
    }

    Processo *p = &processos[pedido.id_aplicacao - 1];
    p->pc = pedido.pc;
    p->n = pedido.n;

    printf("[Kernel] Recebeu %s de A%d (PC=%d, N=%d)\n",
           pedido.operacao == ENVIAR ? "SEND" : "RECV",
           p->id, pedido.pc, pedido.n);

    /* TODO: salvar contexto e parametros, SIGSTOP e enfileirar R/W. */
}

/*
 * Lemos uma mensagem inteira, mesmo que read retorne menos bytes.
 * Os dois canais carregam mensagens de tamanho fixo.
 * Retorno: 1 = sucesso; 0 = pipe fechado; -1 = erro.
 */
static int ler_mensagem(int fd, void *destino, size_t tamanho) {
    size_t recebidos = 0;

    while (recebidos < tamanho) {
        ssize_t n = read(fd, (char *)destino + recebidos,
                         tamanho - recebidos);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("Kernel: read");
            return -1;
        }
        recebidos += (size_t)n;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    /* FD das IRQs, FD das syscalls, depois os seis PIDs das aplicacoes. */
    if (argc != QUANTIDADE_APLICACOES + 3) {
        fprintf(stderr,
                "Uso: KernelSim FD_IRQ FD_SYSCALL PID_A1 ... PID_A6\n");
        return 1;
    }

    setbuf(stdout, NULL);
    int fd_irq = atoi(argv[1]);
    int fd_syscall = atoi(argv[2]);
    iniciar_processos(&argv[3]);

    /* poll permite esperar nos DOIS pipes sem travar em apenas um deles. */
    struct pollfd entradas[2] = {
        { .fd = fd_irq, .events = POLLIN },
        { .fd = fd_syscall, .events = POLLIN }
    };

    escalonar(); /* Inicia A1. */

    for (;;) {
        int prontos = poll(entradas, 2, -1);
        if (prontos == -1) {
            if (errno == EINTR) continue;
            perror("Kernel: poll");
            break;
        }

        /* Verifica as syscalls que chegaram das aplicacoes. */
        if (entradas[1].revents & POLLIN) {
            PedidoSyscall pedido;
            int resultado = ler_mensagem(fd_syscall, &pedido, sizeof pedido);
            if (resultado < 0) break;
            if (resultado == 0) {
                /* Todas as aplicacoes fecharam a escrita desse pipe. */
                close(fd_syscall);
                fd_syscall = -1;
                entradas[1].fd = -1;
            } else {
                tratar_syscall(pedido);
            }
        }

        /* IRQ0 continua escalonando; IRQ1/IRQ2 ainda sao placeholders. */
        if (entradas[0].revents & POLLIN) {
            MensagemIRQ mensagem;
            int resultado = ler_mensagem(fd_irq, &mensagem, sizeof mensagem);
            if (resultado <= 0) break;
            tratar_irq(mensagem);
        }

        if (entradas[0].revents & (POLLERR | POLLHUP | POLLNVAL)) break;
        if (fd_syscall != -1 &&
            (entradas[1].revents & (POLLERR | POLLHUP | POLLNVAL))) {
            /* A escrita do pipe acabou; IRQs podem continuar chegando. */
            close(fd_syscall);
            fd_syscall = -1;
            entradas[1].fd = -1;
        }
    }

    close(fd_irq);
    if (fd_syscall != -1) close(fd_syscall);
    return 0;
}
