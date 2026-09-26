#pragma once

#include <sys/types.h>

/* Constantes e tipos compartilhados pelos programas do simulador. */
enum {
    QUANTIDADE_APLICACOES = 6,
    MAX_ITERACOES = 5000
};

typedef enum {
    PRONTO,
    EXECUTANDO,
    BLOQUEADO_LEITURA,
    BLOQUEADO_ESCRITA,
    TERMINADO
} EstadoProcesso;

typedef enum { NENHUMA_OPERACAO, RECEBER, ENVIAR } Operacao;
typedef enum { IRQ0, IRQ1, IRQ2 } TipoIRQ;

/* Os dois produtores escrevem na MESMA pipe, informando o tipo. */
typedef enum { EVENTO_INTERRUPCAO, EVENTO_SYSCALL, EVENTO_TERMINO } TipoEvento;

typedef struct {
    int id_aplicacao;
    Operacao operacao;
    int pc;
    int n;
} PedidoSyscall;

typedef struct {
    TipoEvento tipo;
    TipoIRQ irq;        /* Utilizado somente se tipo = EVENTO_INTERRUPCAO. */
    /* EVENTO_SYSCALL: operacao, PC e N da syscall.
       EVENTO_TERMINO: id_aplicacao, PC e N finais; operacao = NENHUMA. */
    PedidoSyscall pedido;
} MensagemControle;

typedef struct {
    int id_aplicacao;
    /* NENHUMA_OPERACAO e usada para confirmar EVENTO_TERMINO. */
    Operacao operacao;
    int n;
} RespostaSyscall;

typedef struct {
    int id;
    pid_t pid;
    int pc;
    int n;
    EstadoProcesso estado;
    Operacao operacao_pendente;
    int leituras;
    int escritas;
} Processo;
