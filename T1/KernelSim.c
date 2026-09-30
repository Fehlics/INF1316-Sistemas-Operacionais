#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
===============================================================================
KernelSim.c
===============================================================================

Este e o "nucleo" do trabalho. Ele:
- guarda o PCB das seis Applications;
- aplica Round Robin;
- bloqueia processos que pedem SEND/RECV;
- mantem duas filas FIFO de pedidos pendentes;
- simula os seis buffers direcionais dos tres pares;
- atende IRQ0, IRQ1 e IRQ2;
- controla a pausa e a retomada;
- reconhece o termino das Applications.

Mapa mental para a apresentacao:
IRQ0 -> escalonar()
PEDIDO -> pedido()
IRQ1/IRQ2 -> concluir()
FIM -> terminar()
Ctrl+Z -> foto()
===============================================================================
*/

/* PCB das seis Applications. O indice 0 representa A1, ..., 5 representa A6. */
Processo p[TOTAL];
/* Uma pipe de resposta exclusiva para cada Application. */
int respostas[TOTAL];
/* Pipe usada para enviar fotografias do estado ao Simulador. */
int estado_fd;
/* Indice da Application atualmente na CPU; -1 significa nenhuma. */
int atual = -1;
/* Guarda quem foi o ultimo escalonado para continuar o Round Robin. */
int ultimo = -1;
int pausado = 0;
int terminados = 0;
/*
Duas filas FIFO:
fila[0] = pedidos de RECEBER
fila[1] = pedidos de ENVIAR
*/
int fila[2][TOTAL];
/* Posicao do primeiro elemento valido de cada fila circular. */
int inicio[2];
/* Quantos elementos existem em cada fila. */
int quantidade[2];
/*
Seis buffers direcionais:
buffer[0] guarda dados enviados por A1, buffer[1] por A2, etc.
O parceiro le o buffer do outro processo do par.
*/
int buffer[TOTAL][MAX];
/* Posicao do primeiro dado ainda nao lido de cada buffer. */
int pos[TOTAL];
/* Quantidade de inteiros atualmente guardados em cada buffer. */
int tamanho[TOTAL];
/*
Marca Applications que terminaram durante uma pausa e precisam receber
SIGCONT depois da retomada para conseguir ler a confirmacao de termino.
*/
int confirmar_depois[TOTAL];

/*
Round Robin:
1. Se havia uma Application executando, ela e interrompida e volta a PRONTO.
2. A busca comeca depois da ultima Application escolhida.
3. A primeira PRONTO encontrada recebe SIGCONT e passa a EXECUTANDO.
*/
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

/* Copia os seis PCBs e envia a fotografia para o Simulador. */
void foto(int fase) {
    Fotografia f = { .fase = fase, .atual = atual };
    for (int i = 0; i < TOTAL; i++)
        f.processos[i] = p[i];

    write(estado_fd, &f, sizeof f);
}

/*
Trata SEND/RECV pedido por uma Application.
O processo e colocado na fila FIFO correspondente e passa a BLOQUEADO.
*/
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

    /* Calcula o fim da fila circular e acrescenta o novo pedido. */
    int fim = (inicio[tipo] + quantidade[tipo]) % TOTAL;
    fila[tipo][fim] = i;
    quantidade[tipo]++;
    p[i].pc = m.pc;
    p[i].n = m.n;
    p[i].op = m.op;
    p[i].endereco = m.endereco;
    p[i].estado = BLOQUEADO;
    /* A syscall bloqueia de verdade o processo Unix. */
    kill(p[i].pid, SIGSTOP);

    if (atual == i) {
        atual = -1;
        escalonar();
    }
}

/*
Conclui o primeiro pedido da fila indicada pela interrupcao:
- ENVIAR/IRQ2: grava o PC no buffer do remetente.
- RECEBER/IRQ1: retira o dado mais antigo do buffer do parceiro.
*/
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

        /* Insere o PC no fim do buffer circular do remetente. */
        int fim = (pos[i] + tamanho[i]) % MAX;
        buffer[i][fim] = p[i].pc;
        tamanho[i]++;
        p[i].escritas++;
        printf("SEND A%d PC=%d\n", i + 1, p[i].pc);
    } else {
        /*
        Os pares sao (0,1), (2,3) e (4,5), equivalentes a
        (A1,A2), (A3,A4) e (A5,A6).
        */
        int parceiro = i % 2 == 0 ? i + 1 : i - 1;
        if (tamanho[parceiro] > 0) {
            p[i].n = buffer[parceiro][pos[parceiro]];
            pos[parceiro] = (pos[parceiro] + 1) % MAX;
            tamanho[parceiro]--;
        } else {
            /* RECV sem dado disponivel retorna 0, como pede o enunciado. */
            p[i].n = 0;
        }
        p[i].leituras++;
        printf("RECV A%d N=%d\n", i + 1, p[i].n);
    }

    inicio[tipo] = (inicio[tipo] + 1) % TOTAL;
    quantidade[tipo]--;
    /*
    A resposta libera a Application que estava esperando na sua pipe.
    PC e N representam o contexto devolvido pelo KernelSim.
    */
    Resposta r = { .op = op, .pc = p[i].pc, .n = p[i].n };
    write(respostas[i], &r, sizeof r);
    p[i].op = NENHUMA;
    p[i].endereco = SEM_ENDERECO;
    p[i].estado = PRONTO;

    if (atual < 0)
        escalonar();
}

/*
Registra o termino de uma Application e envia a confirmacao final.
Depois disso, o processo nao participa mais do escalonamento.
*/
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

/*
Argumentos recebidos do Simulador:
argv[1]     = pipe principal de controle
argv[2..7]  = seis pipes de resposta
argv[8..13] = PIDs das seis Applications
argv[14]    = pipe usada para enviar fotografias de estado
*/
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
    /*
    Loop central do KernelSim.
    Toda decisao nasce de uma Mensagem recebida pela pipe de controle.
    */
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
            /* Congela o escalonamento e para quem estava executando. */
            pausado = 1;
            if (atual >= 0)
                kill(p[atual].pid, SIGSTOP);
            foto(1);
        } else if (m.tipo == MOSTRAR && pausado) {
            /* Envia a fotografia definitiva depois que a parada foi confirmada. */
            foto(2);
        } else if (m.tipo == RETOMAR && pausado) {
            /* Reativa o estado anterior e volta a permitir escalonamento. */
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
