#pragma once

#include <sys/types.h>

/*
 * Tipos compartilhados entre os quatro programas.
 * Usamos enum em vez de #define para definir as constantes.
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

/* InterController -> KernelSim (primeiro pipe real). */
typedef struct {
    TipoIRQ tipo;
} MensagemIRQ;

/*
 * Application -> KernelSim (segundo pipe real).
 * Os campos pc e n representam os dados da aplicacao no instante do pedido.
 * Nao enviamos &PC ou &N: ponteiros locais nao sao validos em outro processo.
 * Mais adiante sera necessario um canal de resposta para entregar o N do RECV.
 */
typedef struct {
    int id_aplicacao;
    Operacao operacao;
    int pc;
    int n;
} PedidoSyscall;

/* Informacoes mantidas pelo KernelSim sobre cada aplicacao. */
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
