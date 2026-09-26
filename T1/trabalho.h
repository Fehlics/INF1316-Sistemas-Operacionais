#pragma once

#include <sys/types.h>

/*
 * Estruturas e tipos compartilhados pelos programas.
 * Usamos enum em vez de #define para definir constantes.
 */
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

typedef enum {
    NENHUMA_OPERACAO,
    RECEBER,
    ENVIAR
} Operacao;

typedef enum {
    IRQ0,
    IRQ1,
    IRQ2
} TipoIRQ;

/* Mensagem enviada pelo InterController ao KernelSim (pipe Unix real). */
typedef struct {
    TipoIRQ tipo;
} MensagemIRQ;

/*
 * Modelo de uma futura solicitação de syscall de uma aplicação.
 * O endereço real de &PC ou &N não deve ser enviado entre processos:
 * cada processo Unix tem seu próprio espaço de endereçamento.
 */
typedef struct {
    int id_aplicacao; /* A1 = 1, ..., A6 = 6 */
    Operacao operacao;
    int pc;           /* Valor do contador em caso de ENVIAR. */
} PedidoSyscall;

/* Informações que o KernelSim deverá manter sobre cada aplicação. */
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
