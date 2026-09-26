#pragma once

#include <sys/types.h>

/* Constantes declaradas com enum, sem #define. */
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

/* InterController -> KernelSim: interrupcao gerada. */
typedef struct {
    TipoIRQ tipo;
} MensagemIRQ;

/* Application -> KernelSim: parametros salvos no instante da syscall. */
typedef struct {
    int id_aplicacao;
    Operacao operacao;
    int pc;
    int n;
} PedidoSyscall;

/* KernelSim -> Application: permite concluir a syscall depois do IRQ. */
typedef struct {
    int id_aplicacao;
    Operacao operacao;
    int n; /* Resultado de RECEBER; para ENVIAR, N nao muda. */
} RespostaSyscall;

/* Contexto que o kernel simulado mantem para cada aplicacao. */
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
