#pragma once
#include <sys/types.h>

enum { TOTAL = 6, MAX = 5000 };
enum { IRQ = 1, PEDIDO, CONTEXTO, FIM, PAUSAR, MOSTRAR, RETOMAR };
enum { NENHUMA, RECEBER, ENVIAR };
enum { SEM_ENDERECO, END_PC, END_N };
enum { PRONTO, EXECUTANDO, BLOQUEADO, TERMINADO };

typedef struct {
    int tipo;
    int id;
    int op;
    int pc;
    int n;
    int endereco;
} Mensagem;

typedef struct {
    int op;
    int pc;
    int n;
} Resposta;

typedef struct {
    pid_t pid;
    int pc;
    int n;
    int estado;
    int op;
    int endereco;
    int leituras;
    int escritas;
} Processo;

typedef struct {
    int fase;
    int atual;
    Processo processos[TOTAL];
} Fotografia;
