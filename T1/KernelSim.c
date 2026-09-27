#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Processo p[TOTAL];
int respostas[TOTAL];
int estado_fd;
int atual = -1;
int ultimo = -1;
int pausado = 0;
int terminados = 0;
int fila[2][TOTAL];
int inicio[2];
int quantidade[2];
int buffer[TOTAL][MAX];
int pos[TOTAL];
int tamanho[TOTAL];
int confirmar_depois[TOTAL];

void escalonar(void) {
    if (pausado)
        return;

    if (atual >= 0 && p[atual].estado == EXECUTANDO) {
        kill(p[atual].pid, SIGSTOP);
        p[atual].estado = PRONTO;
    }

    atual = -1;
    for (int passo = 1; passo <= TOTAL; passo++) {
        int i = (ultimo + passo) % TOTAL;
        if (p[i].estado == PRONTO) {
            atual = ultimo = i;
            p[i].estado = EXECUTANDO;
            kill(p[i].pid, SIGCONT);
            printf("CPU A%d\n", i + 1);
            return;
        }
    }
}

void foto(int fase) {
    Fotografia f = { .fase = fase, .atual = atual };
    for (int i = 0; i < TOTAL; i++)
        f.processos[i] = p[i];

    write(estado_fd, &f, sizeof f);
}

void pedido(Mensagem m) {
    int i = m.id - 1;
    if (i < 0 || i >= TOTAL)
        return;
    if (m.op != RECEBER && m.op != ENVIAR)
        return;
    if (m.op == RECEBER && m.endereco != END_N)
        return;
    if (m.op == ENVIAR && m.endereco != END_PC)
        return;

    int tipo = 0;
    if (m.op == ENVIAR)
        tipo = 1;
    if (p[i].estado != PRONTO && p[i].estado != EXECUTANDO)
        return;
    if (quantidade[tipo] == TOTAL)
        return;

    int fim = (inicio[tipo] + quantidade[tipo]) % TOTAL;
    fila[tipo][fim] = i;
    quantidade[tipo]++;
    p[i].pc = m.pc;
    p[i].n = m.n;
    p[i].op = m.op;
    p[i].endereco = m.endereco;
    p[i].estado = BLOQUEADO;
    kill(p[i].pid, SIGSTOP);

    if (atual == i) {
        atual = -1;
        escalonar();
    }
}

void concluir(int op) {
    int tipo = 0;
    if (op == ENVIAR)
        tipo = 1;
    if (!quantidade[tipo])
        return;

    int i = fila[tipo][inicio[tipo]];
    if (op == ENVIAR) {
        if (tamanho[i] == MAX)
            return;

        int fim = (pos[i] + tamanho[i]) % MAX;
        buffer[i][fim] = p[i].pc;
        tamanho[i]++;
        p[i].escritas++;
        printf("SEND A%d PC=%d\n", i + 1, p[i].pc);
    } else {
        int parceiro = i % 2 == 0 ? i + 1 : i - 1;
        if (tamanho[parceiro] > 0) {
            p[i].n = buffer[parceiro][pos[parceiro]];
            pos[parceiro] = (pos[parceiro] + 1) % MAX;
            tamanho[parceiro]--;
        } else {
            p[i].n = 0;
        }
        p[i].leituras++;
        printf("RECV A%d N=%d\n", i + 1, p[i].n);
    }

    inicio[tipo] = (inicio[tipo] + 1) % TOTAL;
    quantidade[tipo]--;
    Resposta r = { .op = op, .pc = p[i].pc, .n = p[i].n };
    write(respostas[i], &r, sizeof r);
    p[i].op = NENHUMA;
    p[i].endereco = SEM_ENDERECO;
    p[i].estado = PRONTO;

    if (atual < 0)
        escalonar();
}

void terminar(Mensagem m) {
    int i = m.id - 1;
    if (i < 0 || i >= TOTAL || p[i].estado == TERMINADO || p[i].estado == BLOQUEADO)
        return;

    int era_atual = atual == i;
    int era_pronto = p[i].estado == PRONTO;
    p[i].pc = m.pc;
    p[i].n = m.n;
    p[i].estado = TERMINADO;
    p[i].op = NENHUMA;
    p[i].endereco = SEM_ENDERECO;
    terminados++;
    printf("TERMINOU A%d PC=%d N=%d L=%d E=%d\n", i + 1, m.pc, m.n, p[i].leituras, p[i].escritas);

    Resposta r = { .op = NENHUMA, .pc = m.pc, .n = m.n };
    write(respostas[i], &r, sizeof r);
    if (pausado)
        confirmar_depois[i] = 1;
    else if (era_pronto)
        kill(p[i].pid, SIGCONT);

    if (era_atual) {
        atual = -1;
        escalonar();
    }

    if (terminados == TOTAL)
        puts("TODOS TERMINARAM");
}

int main(int argc, char **argv) {
    if (argc != 15)
        return 1;

    setbuf(stdout, NULL);
    signal(SIGPIPE, SIG_IGN);
    int controle = atoi(argv[1]);
    for (int i = 0; i < TOTAL; i++) {
        respostas[i] = atoi(argv[2 + i]);
        p[i].pid = (pid_t)atol(argv[8 + i]);
        p[i].estado = PRONTO;
    }

    estado_fd = atoi(argv[14]);
    escalonar();
    Mensagem m;
    while (read(controle, &m, sizeof m) == sizeof m) {
        if (m.tipo == IRQ && !pausado) {
            if (m.op == NENHUMA)
                escalonar();
            else if (m.op == RECEBER || m.op == ENVIAR)
                concluir(m.op);
        } else if (m.tipo == PEDIDO) {
            pedido(m);
        } else if (m.tipo == CONTEXTO && m.id >= 1 && m.id <= TOTAL) {
            int i = m.id - 1;
            if (p[i].estado == TERMINADO)
                continue;

            p[i].pc = m.pc;
            p[i].n = m.n;
        } else if (m.tipo == FIM) {
            terminar(m);
        } else if (m.tipo == PAUSAR && !pausado) {
            pausado = 1;
            if (atual >= 0)
                kill(p[atual].pid, SIGSTOP);
            foto(1);
        } else if (m.tipo == MOSTRAR && pausado) {
            foto(2);
        } else if (m.tipo == RETOMAR && pausado) {
            pausado = 0;
            for (int i = 0; i < TOTAL; i++) {
                if (confirmar_depois[i]) {
                    kill(p[i].pid, SIGCONT);
                    confirmar_depois[i] = 0;
                }
            }
            if (atual >= 0 && p[atual].estado == EXECUTANDO)
                kill(p[atual].pid, SIGCONT);
            else
                escalonar();
            foto(3);
        }
    }

    close(controle);
    close(estado_fd);
    for (int i = 0; i < TOTAL; i++)
        close(respostas[i]);
    return 0;
}
