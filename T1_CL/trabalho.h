#pragma once
#include <sys/types.h>

/* Tipos e constantes usados pelos quatro programas do trabalho. */

enum { QUANTIDADE_APLICACOES = 6, MAX_ITERACOES = 5000, CAP_BUFFER = 64 };

typedef enum { PRONTO, EXECUTANDO, BLOQUEADO, TERMINADO } EstadoProcesso;
typedef enum { NENHUMA, RECEBER, ENVIAR } Operacao;
typedef enum { IRQ0, IRQ1, IRQ2 } TipoIRQ;

/* Tudo o que trafega pela pipe de controle (aplicacoes + InterController). */
typedef enum {
    EVT_INTERRUPCAO, EVT_SYSCALL, EVT_TERMINO,
    EVT_CONTEXTO, EVT_PAUSAR, EVT_RETOMAR
} TipoEvento;

typedef struct {
    int id;
    Operacao operacao; /* usado em EVT_SYSCALL e para indicar bloqueio */
    int pc;
    int n;
} PedidoSyscall;

typedef struct {
    TipoEvento tipo;
    TipoIRQ irq;         /* valido apenas se tipo == EVT_INTERRUPCAO */
    PedidoSyscall dado;  /* valido nos demais tipos de evento */
} MensagemControle;

typedef struct {
    int id;
    Operacao operacao;
    int pc, n;
} RespostaSyscall;

/* PCB simulado de cada aplicacao, mantido pelo KernelSim. */
typedef struct {
    int id;
    pid_t pid;
    int pc, n;
    EstadoProcesso estado;
    Operacao operacao_pendente; /* motivo do bloqueio, se BLOQUEADO */
    int leituras, escritas;
} Processo;

/* Fotografia enviada ao Simulador quando o sistema e pausado (Ctrl+Z). */
typedef struct {
    int executando; /* id (1..6) de quem estava na CPU, ou 0 */
    Processo processos[QUANTIDADE_APLICACOES];
} EstadoSimulador;
