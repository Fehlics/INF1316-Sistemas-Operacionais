#include "trabalho.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Processo processos[QUANTIDADE_APLICACOES];
static int atual = -1;
static int ultimo_escalonado = -1;

/* Duas filas FIFO, cada uma guarda indices dos processos bloqueados. */
typedef struct {
    int indices[QUANTIDADE_APLICACOES];
    int inicio;
    int quantidade;
} FilaBloqueados;
static FilaBloqueados fila_leitura, fila_escrita;
static int fd_respostas[QUANTIDADE_APLICACOES];

/*
 * Tres pipes bidirecionais, cada um com dois buffers de inteiros.
 * O indice do buffer e o indice do REMETENTE:
 * 0: A1 -> A2, 1: A2 -> A1, 2: A3 -> A4, 3: A4 -> A3,
 * 4: A5 -> A6, 5: A6 -> A5.
 *
 * Uma aplicacao executa no maximo MAX_ITERACOES-1 passos;
 * por isso esse tamanho comporta todas as escritas que ela pode pedir.
 */
typedef struct {
    int dados[MAX_ITERACOES];
    int inicio;
    int quantidade;
} BufferPipe;

static BufferPipe buffers[QUANTIDADE_APLICACOES];

/* A1 faz par com A2, A3 com A4 e A5 com A6. Indices: 0 a 5. */
static int indice_parceiro(int indice) {
    return indice % 2 == 0 ? indice + 1 : indice - 1;
}

/* Adiciona um contador ao final do buffer do remetente (ordem FIFO). */
static int guardar_pc(int remetente, int pc) {
    BufferPipe *buffer = &buffers[remetente];
    if (buffer->quantidade == MAX_ITERACOES) return 0;
    int fim = (buffer->inicio + buffer->quantidade) % MAX_ITERACOES;
    buffer->dados[fim] = pc;
    buffer->quantidade++;
    return 1;
}

/* Retira o valor mais antigo enviado pelo parceiro ou retorna zero. */
static int receber_pc(int destinatario) {
    int remetente = indice_parceiro(destinatario);
    BufferPipe *buffer = &buffers[remetente];
    if (buffer->quantidade == 0) return 0;
    int pc = buffer->dados[buffer->inicio];
    buffer->inicio = (buffer->inicio + 1) % MAX_ITERACOES;
    buffer->quantidade--;
    return pc;
}


static int enfileirar(FilaBloqueados *fila, int indice) {
    if (fila->quantidade == QUANTIDADE_APLICACOES) return 0;
    int fim = (fila->inicio + fila->quantidade) % QUANTIDADE_APLICACOES;
    fila->indices[fim] = indice;
    fila->quantidade++;
    return 1;
}

static int desenfileirar(FilaBloqueados *fila) {
    if (fila->quantidade == 0) return -1;
    int indice = fila->indices[fila->inicio];
    fila->inicio = (fila->inicio + 1) % QUANTIDADE_APLICACOES;
    fila->quantidade--;
    return indice;
}

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
    atual = -1;
    for (int passo = 1; passo <= QUANTIDADE_APLICACOES; passo++) {
        int proximo = (ultimo_escalonado + passo) % QUANTIDADE_APLICACOES;
        if (processos[proximo].estado == PRONTO) {
            atual = proximo;
            ultimo_escalonado = proximo;
            processos[atual].estado = EXECUTANDO;
            kill(processos[atual].pid, SIGCONT);
            printf("[Kernel] Executando A%d\n", processos[atual].id);
            return;
        }
    }
    puts("[Kernel] Nenhuma aplicacao pronta.");
}

/* Em cada syscall, salva o contexto, suspende e enfileira a aplicacao. */
static void tratar_syscall(PedidoSyscall pedido) {
    if (pedido.id_aplicacao < 1 ||
        pedido.id_aplicacao > QUANTIDADE_APLICACOES ||
        (pedido.operacao != ENVIAR && pedido.operacao != RECEBER)) {
        puts("[Kernel] Pedido de syscall invalido.");
        return;
    }
    int indice = pedido.id_aplicacao - 1;
    Processo *p = &processos[indice];
    if (p->estado != PRONTO && p->estado != EXECUTANDO) {
        printf("[Kernel] Ignorou syscall de A%d: nao esta pronta.\n", p->id);
        return;
    }

    FilaBloqueados *fila = pedido.operacao == ENVIAR ?
                            &fila_escrita : &fila_leitura;
    if (!enfileirar(fila, indice)) {
        puts("[Kernel] Fila de bloqueados cheia.");
        return;
    }
    p->pc = pedido.pc;
    p->n = pedido.n;
    p->operacao_pendente = pedido.operacao;
    p->estado = pedido.operacao == ENVIAR ?
                BLOQUEADO_ESCRITA : BLOQUEADO_LEITURA;
    kill(p->pid, SIGSTOP);
    printf("[Kernel] Bloqueou A%d (%s, PC=%d, N=%d; fila=%d)\n",
           p->id, pedido.operacao == ENVIAR ? "SEND" : "RECV",
           p->pc, p->n, fila->quantidade);
    if (atual == indice) {
        atual = -1;
        escalonar();
    }
}

/* IRQ1 e IRQ2 concluem sempre o primeiro pedido da respectiva fila. */
static void concluir_syscall(FilaBloqueados *fila, Operacao operacao) {
    if (fila->quantidade == 0) {
        printf("[Kernel] IRQ%d ignorada: fila vazia.\n",
               operacao == RECEBER ? 1 : 2);
        return;
    }

    /* Consultamos a primeira posicao sem retira-la ainda. Assim, se um
     * buffer estiver cheio, a escrita permanece bloqueada e nao se perde. */
    int indice = fila->indices[fila->inicio];
    Processo *p = &processos[indice];
    if (p->operacao_pendente != operacao) {
        fprintf(stderr, "[Kernel] Erro: operacao na fila inconsistente.\n");
        return;
    }

    if (operacao == ENVIAR) {
        if (!guardar_pc(indice, p->pc)) {
            printf("[Kernel] Buffer de A%d cheio; SEND continua bloqueado.\n",
                   p->id);
            return;
        }
        printf("[Kernel] A%d enviou PC=%d para A%d (%d no buffer).\n",
               p->id, p->pc, indice_parceiro(indice) + 1,
               buffers[indice].quantidade);
    } else {
        p->n = receber_pc(indice);
        printf("[Kernel] A%d recebeu N=%d de A%d (%d no buffer).\n",
               p->id, p->n, indice_parceiro(indice) + 1,
               buffers[indice_parceiro(indice)].quantidade);
    }

    desenfileirar(fila);
    RespostaSyscall resposta = {
        .id_aplicacao = p->id,
        .operacao = operacao,
        .n = p->n
    };
    ssize_t escritos;
    do {
        escritos = write(fd_respostas[indice], &resposta, sizeof resposta);
    } while (escritos == -1 && errno == EINTR);
    if (escritos != (ssize_t)sizeof resposta) {
        perror("Kernel: write resposta");
        return;
    }
    if (operacao == RECEBER) p->leituras++;
    else p->escritas++;
    p->operacao_pendente = NENHUMA_OPERACAO;
    p->estado = PRONTO;
    printf("[Kernel] Concluiu %s de A%d; A%d agora PRONTO.\n",
           operacao == ENVIAR ? "SEND" : "RECV", p->id, p->id);
    if (atual == -1) escalonar();
}

static void tratar_irq(MensagemIRQ mensagem) {
    switch (mensagem.tipo) {
        case IRQ0: escalonar(); break;
        case IRQ1: concluir_syscall(&fila_leitura, RECEBER); break;
        case IRQ2: concluir_syscall(&fila_escrita, ENVIAR); break;
        default: puts("[Kernel] IRQ desconhecida.");
    }
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
    if (argc != QUANTIDADE_APLICACOES * 2 + 3) {
        fprintf(stderr,
                "Uso: KernelSim FD_IRQ FD_SYSCALL FD_RES_A1..A6 PID_A1..A6\n");
        return 1;
    }

    setbuf(stdout, NULL);
    int fd_irq = atoi(argv[1]);
    int fd_syscall = atoi(argv[2]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        fd_respostas[i] = atoi(argv[3 + i]);
    iniciar_processos(&argv[3 + QUANTIDADE_APLICACOES]);

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

        /* IRQ0 escalona; IRQ1 e IRQ2 liberam filas FIFO. */
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
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) close(fd_respostas[i]);
    return 0;
}
