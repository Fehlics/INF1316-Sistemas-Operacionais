#include "trabalho.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct { int itens[QUANTIDADE_APLICACOES], inicio, qtd; } Fila;
typedef struct { int dados[CAP_BUFFER], inicio, qtd; } Buffer;

static Processo processos[QUANTIDADE_APLICACOES];
static Fila fila_leitura, fila_escrita;
static Buffer buffers[QUANTIDADE_APLICACOES]; /* buffer[i] = o que Ai+1 enviou */
static int respostas[QUANTIDADE_APLICACOES];
static int atual = -1, ultimo = -1, terminados = 0, pausado = 0, fd_status;

static int parceiro(int i) { return i % 2 == 0 ? i + 1 : i - 1; }

static void enfileirar(Fila *f, int i) {
    f->itens[(f->inicio + f->qtd) % QUANTIDADE_APLICACOES] = i;
    f->qtd++;
}
static int desenfileirar(Fila *f) {
    int i = f->itens[f->inicio];
    f->inicio = (f->inicio + 1) % QUANTIDADE_APLICACOES;
    f->qtd--;
    return i;
}
static void guardar(Buffer *b, int v) {
    if (b->qtd == CAP_BUFFER) return;
    b->dados[(b->inicio + b->qtd) % CAP_BUFFER] = v;
    b->qtd++;
}
static int retirar(Buffer *b) {
    if (b->qtd == 0) return 0; /* modo NO_WAIT: nada a receber */
    int v = b->dados[b->inicio];
    b->inicio = (b->inicio + 1) % CAP_BUFFER;
    b->qtd--;
    return v;
}

/* Round Robin: para o atual, ativa o proximo PRONTO na ordem circular. */
static void escalonar(void) {
    if (pausado) return;
    if (atual != -1) {
        kill(processos[atual].pid, SIGSTOP);
        processos[atual].estado = PRONTO;
        atual = -1;
    }
    for (int p = 1; p <= QUANTIDADE_APLICACOES; p++) {
        int i = (ultimo + p) % QUANTIDADE_APLICACOES;
        if (processos[i].estado == PRONTO) {
            atual = ultimo = i;
            processos[i].estado = EXECUTANDO;
            kill(processos[i].pid, SIGCONT);
            printf("[Kernel] Executando A%d\n", processos[i].id);
            return;
        }
    }
}

static void ao_contexto(PedidoSyscall d) {
    Processo *p = &processos[d.id - 1];
    if (p->estado == TERMINADO) return;
    p->pc = d.pc;
    p->n = d.n;
}

static void ao_syscall(PedidoSyscall d) {
    Processo *p = &processos[d.id - 1];
    if (p->estado != PRONTO && p->estado != EXECUTANDO) return;
    Fila *f = d.operacao == ENVIAR ? &fila_escrita : &fila_leitura;
    enfileirar(f, d.id - 1);
    p->pc = d.pc;
    p->n = d.n;
    p->operacao_pendente = d.operacao;
    p->estado = BLOQUEADO;
    kill(p->pid, SIGSTOP);
    printf("[Kernel] Bloqueou A%d (%s)\n", p->id,
           d.operacao == ENVIAR ? "SEND" : "RECV");
    if (atual == d.id - 1) { atual = -1; escalonar(); }
}

/* Atende o primeiro da fila (IRQ1 = fim de RECV, IRQ2 = fim de SEND). */
static void concluir(Fila *f, Operacao op) {
    if (f->qtd == 0) return; /* interrupcao perdida: ninguem esperando */
    int i = f->itens[f->inicio];
    Processo *p = &processos[i];
    if (op == ENVIAR) guardar(&buffers[i], p->pc);
    else p->n = retirar(&buffers[parceiro(i)]);
    desenfileirar(f);

    RespostaSyscall r = {p->id, op, p->pc, p->n};
    write(respostas[i], &r, sizeof r);
    if (op == ENVIAR) p->escritas++; else p->leituras++;
    p->operacao_pendente = NENHUMA;
    p->estado = PRONTO;
    printf("[Kernel] Concluiu %s de A%d\n", op == ENVIAR ? "SEND" : "RECV", p->id);
    if (atual == -1) escalonar();
}

static void ao_termino(PedidoSyscall d) {
    Processo *p = &processos[d.id - 1];
    if (p->estado == TERMINADO || p->estado == BLOQUEADO) return;
    int estava_executando = atual == d.id - 1;
    p->pc = d.pc;
    p->n = d.n;
    p->estado = TERMINADO;
    terminados++;
    printf("[Kernel] A%d TERMINADO (PC=%d, leituras=%d, escritas=%d)\n",
           p->id, p->pc, p->leituras, p->escritas);

    RespostaSyscall r = {p->id, NENHUMA, p->pc, p->n};
    write(respostas[d.id - 1], &r, sizeof r);
    if (estava_executando) { atual = -1; escalonar(); }
    if (terminados == QUANTIDADE_APLICACOES)
        puts("[Kernel] Todas as seis aplicacoes terminaram.");
}

static void enviar_estado(int executando) {
    EstadoSimulador foto = {0};
    foto.executando = executando;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) foto.processos[i] = processos[i];
    write(fd_status, &foto, sizeof foto);
}

static void ao_pausar(void) {
    if (pausado) return;
    pausado = 1;
    int executando = atual == -1 ? 0 : processos[atual].id;
    if (atual != -1) kill(processos[atual].pid, SIGSTOP);
    enviar_estado(executando);
}

static void ao_retomar(void) {
    if (!pausado) return;
    pausado = 0;
    if (atual != -1) kill(processos[atual].pid, SIGCONT);
    else escalonar();
}

static int ler_evento(int fd, MensagemControle *m) {
    size_t total = 0;
    while (total < sizeof *m) {
        ssize_t n = read(fd, (char *)m + total, sizeof *m - total);
        if (n == 0) return 0;
        if (n < 0) { if (errno == EINTR) continue; return 0; }
        total += (size_t)n;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != QUANTIDADE_APLICACOES * 2 + 3) {
        fprintf(stderr, "Uso: KernelSim FD_CONTROLE RESP_A1..A6 PID_A1..A6 FD_STATUS\n");
        return 1;
    }
    setbuf(stdout, NULL);
    int controle = atoi(argv[1]);
    fd_status = atoi(argv[2 + 2 * QUANTIDADE_APLICACOES]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        respostas[i] = atoi(argv[2 + i]);
        processos[i].id = i + 1;
        processos[i].pid = (pid_t)atol(argv[2 + QUANTIDADE_APLICACOES + i]);
        processos[i].estado = PRONTO;
    }
    escalonar();

    MensagemControle m;
    while (ler_evento(controle, &m)) {
        switch (m.tipo) {
            case EVT_INTERRUPCAO:
                if (!pausado) {
                    if (m.irq == IRQ0) escalonar();
                    else if (m.irq == IRQ1) concluir(&fila_leitura, RECEBER);
                    else concluir(&fila_escrita, ENVIAR);
                }
                break;
            case EVT_SYSCALL: ao_syscall(m.dado); break;
            case EVT_TERMINO: ao_termino(m.dado); break;
            case EVT_CONTEXTO: ao_contexto(m.dado); break;
            case EVT_PAUSAR: ao_pausar(); break;
            case EVT_RETOMAR: ao_retomar(); break;
        }
    }
    return 0;
}
