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
/* Representacao LOGICA do endereco de uma syscall. Um ponteiro real
 * de Application nao pode ser utilizado diretamente por KernelSim,
 * pois os dois sao processos Unix com memorias independentes. */
typedef enum { SEM_ENDERECO, ENDERECO_PC, ENDERECO_N } EnderecoSyscall;
typedef enum { IRQ0, IRQ1, IRQ2 } TipoIRQ;

/* Os dois produtores escrevem na MESMA pipe, informando o tipo. */
typedef enum {
    EVENTO_INTERRUPCAO, EVENTO_SYSCALL, EVENTO_TERMINO,
    EVENTO_CONTEXTO, EVENTO_PAUSAR, EVENTO_MOSTRAR, EVENTO_RETOMAR
} TipoEvento;

typedef struct {
    int id_aplicacao;
    Operacao operacao;
    EnderecoSyscall endereco;
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
    int pc; /* PC salvo no kernel e devolvido ao concluir a syscall. */
} RespostaSyscall;

typedef struct {
    int id;
    pid_t pid;
    int pc;
    int n;
    EstadoProcesso estado;
    Operacao operacao_pendente;
    EnderecoSyscall endereco_pendente;
    int leituras;
    int escritas;
} Processo;

/* KernelSim responde ao Simulador por uma pipe propria. Usamos estruturas
 * simples, assim como na comunicacao de syscalls. */
typedef struct {
    int fase; /* 1 = parou, 2 = fotografia pronta, 3 = retomou */
    int executando; /* ID da aplicacao que ocupava a CPU antes da pausa */
    Processo processos[QUANTIDADE_APLICACOES];
} EstadoSimulador;
