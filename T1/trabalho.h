#pragma once
#include <sys/types.h>

enum { TOTAL = 6, MAX = 5000 };
enum { IRQ = 1, PEDIDO, CONTEXTO, FIM, PAUSAR, MOSTRAR, RETOMAR };
enum { NENHUMA, RECEBER, ENVIAR };
enum { SEM_ENDERECO, END_PC, END_N };
enum { PRONTO, EXECUTANDO, BLOQUEADO, TERMINADO };

typedef struct {
    int tipo, id, op, pc, n, endereco;
} Mensagem;

typedef struct {
    int op, pc, n;
} Resposta;

typedef struct {
    pid_t pid;
    int pc, n, estado, op, endereco, leituras, escritas;
} Processo;

typedef struct {
    int fase, atual;
    Processo processos[TOTAL];
} Fotografia;
