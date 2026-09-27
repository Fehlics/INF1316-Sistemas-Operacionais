#include "trabalho.h"
#include "kernel_filas.h"
#include "util.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Processo processos[QUANTIDADE_APLICACOES];
static FilaBloqueados fila_leitura, fila_escrita;
static int respostas[QUANTIDADE_APLICACOES];
static int atual = -1, ultimo_escalonado = -1;
static int quantidade_terminados, pausado, ultima_ativa_na_pausa;
static int fd_status = -1;
static int confirmar_termino_depois[QUANTIDADE_APLICACOES];
static int passos_conferidos[QUANTIDADE_APLICACOES];
static int contextos_repetidos[QUANTIDADE_APLICACOES];
static int contextos_invalidos[QUANTIDADE_APLICACOES];

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
    if (quantidade_terminados < QUANTIDADE_APLICACOES)
        puts("[Kernel] Nenhuma aplicacao pronta.");
}

static void atualizar_contexto(PedidoSyscall valor) {
    if (valor.id_aplicacao < 1 || valor.id_aplicacao > QUANTIDADE_APLICACOES)
        return;
    Processo *p = &processos[valor.id_aplicacao - 1];
    if (p->estado == TERMINADO) return;
    if (getenv("TESTE_VALIDAR_PC") != NULL) {
        int i = valor.id_aplicacao - 1;
        if (valor.pc == passos_conferidos[i] + 1)
            passos_conferidos[i]++;
        else if (valor.pc == passos_conferidos[i])
            contextos_repetidos[i]++; /* Depois de concluir syscall. */
        else
            contextos_invalidos[i]++;
    }
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
    if (!enviar_dados(fd_status, &fotografia, sizeof fotografia))
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
        (pedido.operacao != ENVIAR && pedido.operacao != RECEBER) ||
        (pedido.operacao == ENVIAR && pedido.endereco != ENDERECO_PC) ||
        (pedido.operacao == RECEBER && pedido.endereco != ENDERECO_N)) {
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
    p->endereco_pendente = pedido.endereco;
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
    RespostaSyscall resposta = {p->id, op, p->n, p->pc};
    if (!enviar_dados(respostas[indice], &resposta, sizeof resposta)) {
        perror("Kernel: resposta");
        return;
    }
    if (op == ENVIAR) p->escritas++;
    else p->leituras++;
    p->operacao_pendente = NENHUMA_OPERACAO;
    p->endereco_pendente = SEM_ENDERECO;
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
    p->endereco_pendente = SEM_ENDERECO;
    p->estado = TERMINADO;
    quantidade_terminados++;
    if (getenv("TESTE_VALIDAR_PC") != NULL) {
        int passou = passos_conferidos[indice] == MAX_ITERACOES &&
                     aviso.pc == MAX_ITERACOES &&
                     contextos_invalidos[indice] == 0;
        printf("[Validacao5000] A%d %s passos=%d repetidos=%d erros=%d PC=%d\n",
               p->id, passou ? "PASSOU" : "FALHOU",
               passos_conferidos[indice], contextos_repetidos[indice],
               contextos_invalidos[indice], aviso.pc);
    }
    printf("[Kernel] A%d TERMINADO (PC=%d, N=%d, leituras=%d, escritas=%d).\n",
           p->id, p->pc, p->n, p->leituras, p->escritas);

    /* Confirma o recebimento, antes de o processo Unix efetuar exit. */
    RespostaSyscall resposta = {p->id, NENHUMA_OPERACAO, p->n, p->pc};
    if (!enviar_dados(respostas[indice], &resposta, sizeof resposta))
        perror("Kernel: confirmacao de termino");

    /* Um processo preemptado pode estar parado apos enviar o aviso. */
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
    if (pausado) return;
    switch (irq) {
        case IRQ0: escalonar(); break;
        case IRQ1: concluir(&fila_leitura, RECEBER); break;
        case IRQ2: concluir(&fila_escrita, ENVIAR); break;
        default: break;
    }
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
    while (receber_dados(controle, &mensagem, sizeof mensagem)) {
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
