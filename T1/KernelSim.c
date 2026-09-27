#include "trabalho.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Filas circulares dos bloqueados, utilizando somente vetores. */
typedef struct {
    int indices[QUANTIDADE_APLICACOES];
    int inicio;
    int quantidade;
} FilaBloqueados;

typedef struct {
    int dados[MAX_ITERACOES];
    int inicio;
    int quantidade;
} BufferPipe;

static Processo processos[QUANTIDADE_APLICACOES];
static FilaBloqueados fila_leitura, fila_escrita;
/* Buffer i contem o que Ai+1 enviou ao parceiro. */
static BufferPipe buffers[QUANTIDADE_APLICACOES];
static int respostas[QUANTIDADE_APLICACOES];
static int atual = -1, ultimo_escalonado = -1;
static int quantidade_terminados = 0;
static int pausado = 0;
static int fd_status = -1;
static int ultima_ativa_na_pausa = 0;
static int confirmar_termino_depois[QUANTIDADE_APLICACOES];

static int parceiro(int indice) {
    return indice % 2 == 0 ? indice + 1 : indice - 1;
}

static int guardar(int remetente, int pc) {
    BufferPipe *b = &buffers[remetente];
    if (b->quantidade == MAX_ITERACOES) return 0;
    b->dados[(b->inicio + b->quantidade) % MAX_ITERACOES] = pc;
    b->quantidade++;
    return 1;
}

static int retirar(int destinatario) {
    BufferPipe *b = &buffers[parceiro(destinatario)];
    if (b->quantidade == 0) return 0;
    int valor = b->dados[b->inicio];
    b->inicio = (b->inicio + 1) % MAX_ITERACOES;
    b->quantidade--;
    return valor;
}

static int enfileirar(FilaBloqueados *f, int indice) {
    if (f->quantidade == QUANTIDADE_APLICACOES) return 0;
    f->indices[(f->inicio + f->quantidade) % QUANTIDADE_APLICACOES] = indice;
    f->quantidade++;
    return 1;
}

static int desenfileirar(FilaBloqueados *f) {
    int indice = f->indices[f->inicio];
    f->inicio = (f->inicio + 1) % QUANTIDADE_APLICACOES;
    f->quantidade--;
    return indice;
}

static void escalonar(void) {
    if (pausado) return;
    if (atual != -1 && processos[atual].estado == EXECUTANDO) {
        kill(processos[atual].pid, SIGSTOP);
        processos[atual].estado = PRONTO;
    }
    atual = -1;
    for (int passo = 1; passo <= QUANTIDADE_APLICACOES; passo++) {
        int indice = (ultimo_escalonado + passo) % QUANTIDADE_APLICACOES;
        if (processos[indice].estado == PRONTO) {
            atual = indice;
            ultimo_escalonado = indice;
            processos[indice].estado = EXECUTANDO;
            kill(processos[indice].pid, SIGCONT);
            printf("[Kernel] Executando A%d\n", processos[indice].id);
            return;
        }
    }
    /* Evita repetir a mesma mensagem a cada IRQ0 depois do ultimo fim. */
    if (quantidade_terminados < QUANTIDADE_APLICACOES)
        puts("[Kernel] Nenhuma aplicacao pronta.");
}

/* Cada aplicacao informa seu PC/N no inicio da iteracao e depois de
 * uma syscall. Assim o PCB nao depende de a aplicacao gerar SEND/RECV. */
static void atualizar_contexto(PedidoSyscall valor) {
    if (valor.id_aplicacao < 1 || valor.id_aplicacao > QUANTIDADE_APLICACOES)
        return;
    Processo *p = &processos[valor.id_aplicacao - 1];
    if (p->estado == TERMINADO) return;
    /* Os eventos de um mesmo remetente chegam na mesma ordem da pipe. */
    p->pc = valor.pc;
    p->n = valor.n;
}

/* Entrega o estado atual ao Simulador. Somente o kernel escreve nessa pipe. */
static void enviar_estado(int fase) {
    if (fd_status < 0) return; /* Os testes mais antigos nao usam status. */
    EstadoSimulador fotografia = {0};
    fotografia.fase = fase;
    fotografia.executando = ultima_ativa_na_pausa;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        fotografia.processos[i] = processos[i];
    ssize_t escritos;
    do { escritos = write(fd_status, &fotografia, sizeof fotografia); }
    while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof fotografia)
        perror("Kernel: estado para Simulador");
}

static void tratar_pausa(void) {
    if (pausado) return;
    pausado = 1;
    ultima_ativa_na_pausa = atual < 0 ? 0 : processos[atual].id;
    if (atual >= 0) kill(processos[atual].pid, SIGSTOP);
    enviar_estado(1); /* Agora o Simulador pode confirmar as paradas. */
}

static void tratar_retomada(void) {
    if (!pausado) return;
    pausado = 0;
    /* Um processo terminado pode ter escrito EVENTO_TERMINO pouco antes
       da pausa: SIGCONT apenas lhe permite ler a confirmacao e sair. */
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (confirmar_termino_depois[i]) {
            kill(processos[i].pid, SIGCONT);
            confirmar_termino_depois[i] = 0;
        }
    }
    if (atual >= 0 && processos[atual].estado == EXECUTANDO)
        kill(processos[atual].pid, SIGCONT);
    else escalonar();
    enviar_estado(3);
}

static void tratar_pedido(PedidoSyscall pedido) {
    if (pedido.id_aplicacao < 1 || pedido.id_aplicacao > QUANTIDADE_APLICACOES ||
        (pedido.operacao != ENVIAR && pedido.operacao != RECEBER)) {
        puts("[Kernel] Pedido invalido.");
        return;
    }
    int indice = pedido.id_aplicacao - 1;
    Processo *p = &processos[indice];
    if (p->estado != PRONTO && p->estado != EXECUTANDO) return;
    FilaBloqueados *f = pedido.operacao == ENVIAR ? &fila_escrita : &fila_leitura;
    if (!enfileirar(f, indice)) return;

    p->pc = pedido.pc;
    p->n = pedido.n;
    p->operacao_pendente = pedido.operacao;
    p->estado = pedido.operacao == ENVIAR ? BLOQUEADO_ESCRITA : BLOQUEADO_LEITURA;
    kill(p->pid, SIGSTOP);
    printf("[Kernel] Bloqueou A%d (%s, PC=%d, N=%d; fila=%d)\n",
           p->id, pedido.operacao == ENVIAR ? "SEND" : "RECV",
           p->pc, p->n, f->quantidade);
    if (atual == indice) {
        atual = -1;
        escalonar();
    }
}

static void concluir(FilaBloqueados *f, Operacao op) {
    if (f->quantidade == 0) {
        printf("[Kernel] IRQ%d ignorada: fila vazia.\n",
               op == RECEBER ? 1 : 2);
        return;
    }
    int indice = f->indices[f->inicio];
    Processo *p = &processos[indice];
    if (p->operacao_pendente != op) return;

    if (op == ENVIAR) {
        if (!guardar(indice, p->pc)) return;
        printf("[Kernel] A%d enviou PC=%d para A%d.\n",
               p->id, p->pc, parceiro(indice) + 1);
    } else {
        p->n = retirar(indice);
        printf("[Kernel] A%d recebeu N=%d de A%d.\n",
               p->id, p->n, parceiro(indice) + 1);
    }
    desenfileirar(f);
    RespostaSyscall resposta = {p->id, op, p->n};
    ssize_t escritos;
    do {
        escritos = write(respostas[indice], &resposta, sizeof resposta);
    } while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof resposta) {
        perror("Kernel: resposta");
        return;
    }
    if (op == ENVIAR) p->escritas++;
    else p->leituras++;
    p->operacao_pendente = NENHUMA_OPERACAO;
    p->estado = PRONTO;
    printf("[Kernel] Concluiu %s de A%d; A%d agora PRONTO.\n",
           op == ENVIAR ? "SEND" : "RECV", p->id, p->id);
    if (atual == -1) escalonar();
}

/* Mensagem de termino voluntario recebida pela mesma pipe de controle. */
static void tratar_termino(PedidoSyscall aviso) {
    if (aviso.id_aplicacao < 1 || aviso.id_aplicacao > QUANTIDADE_APLICACOES ||
        aviso.operacao != NENHUMA_OPERACAO) {
        puts("[Kernel] Aviso de termino invalido.");
        return;
    }
    int indice = aviso.id_aplicacao - 1;
    Processo *p = &processos[indice];
    if (p->estado == TERMINADO) return; /* Evita contar o mesmo fim duas vezes. */
    if (p->estado == BLOQUEADO_LEITURA || p->estado == BLOQUEADO_ESCRITA) {
        printf("[Kernel] Aviso de termino de A%d rejeitado: syscall pendente.\n",
               p->id);
        return;
    }

    int estava_executando = atual == indice;
    int estava_parado = p->estado == PRONTO;
    p->pc = aviso.pc;
    p->n = aviso.n;
    p->operacao_pendente = NENHUMA_OPERACAO;
    p->estado = TERMINADO;
    quantidade_terminados++;
    printf("[Kernel] A%d TERMINADO (PC=%d, N=%d, leituras=%d, escritas=%d).\n",
           p->id, p->pc, p->n, p->leituras, p->escritas);

    /* Confirma o recebimento, antes de o processo Unix efetuar exit. */
    RespostaSyscall resposta = {p->id, NENHUMA_OPERACAO, p->n};
    ssize_t escritos;
    do { escritos = write(respostas[indice], &resposta, sizeof resposta); }
    while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof resposta)
        perror("Kernel: confirmacao de termino");

    /* Um processo preemptado apos escrever o aviso ainda pode estar parado.
       Retoma-o somente para receber a confirmacao e encerrar. */
    if (pausado) confirmar_termino_depois[indice] = 1;
    else if (estava_parado) kill(p->pid, SIGCONT);
    if (estava_executando) {
        atual = -1;
        escalonar();
    }
    if (quantidade_terminados == QUANTIDADE_APLICACOES)
        puts("[Kernel] Todas as seis aplicacoes terminaram.");
}

static void tratar_interrupcao(TipoIRQ irq) {
    /* O controlador estara suspenso durante a pausa. Caso uma mensagem
       seja injetada por teste, ela nao modifica o estado congelado. */
    if (pausado) return;
    switch (irq) {
        case IRQ0: escalonar(); break;
        case IRQ1: concluir(&fila_leitura, RECEBER); break;
        case IRQ2: concluir(&fila_escrita, ENVIAR); break;
        default: break;
    }
}

/* read normal na UNICA pipe compartilhada; sem poll/select/threads. */
static int ler_evento(int fd, MensagemControle *mensagem) {
    size_t total = 0;
    while (total < sizeof *mensagem) {
        ssize_t n = read(fd, (char *)mensagem + total, sizeof *mensagem - total);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("Kernel: read");
            return 0;
        }
        total += (size_t)n;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != QUANTIDADE_APLICACOES * 2 + 2 &&
        argc != QUANTIDADE_APLICACOES * 2 + 3) {
        fprintf(stderr, "Uso: KernelSim FD_CONTROLE RES_A1..A6 PID_A1..A6 [FD_STATUS]\n");
        return 1;
    }
    setbuf(stdout, NULL);
    int controle = atoi(argv[1]);
    if (argc == QUANTIDADE_APLICACOES * 2 + 3)
        fd_status = atoi(argv[2 + 2 * QUANTIDADE_APLICACOES]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        respostas[i] = atoi(argv[2 + i]);
        processos[i].id = i + 1;
        processos[i].pid = (pid_t)atol(argv[2 + QUANTIDADE_APLICACOES + i]);
        processos[i].estado = PRONTO;
    }
    escalonar();
    MensagemControle mensagem;
    while (ler_evento(controle, &mensagem)) {
        if (mensagem.tipo == EVENTO_INTERRUPCAO)
            tratar_interrupcao(mensagem.irq);
        else if (mensagem.tipo == EVENTO_SYSCALL)
            tratar_pedido(mensagem.pedido);
        else if (mensagem.tipo == EVENTO_TERMINO)
            tratar_termino(mensagem.pedido);
        else if (mensagem.tipo == EVENTO_CONTEXTO)
            atualizar_contexto(mensagem.pedido);
        else if (mensagem.tipo == EVENTO_PAUSAR)
            tratar_pausa();
        else if (mensagem.tipo == EVENTO_MOSTRAR && pausado)
            enviar_estado(2);
        else if (mensagem.tipo == EVENTO_RETOMAR)
            tratar_retomada();
    }
    close(controle);
    if (fd_status >= 0) close(fd_status);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) close(respostas[i]);
    return 0;
}
