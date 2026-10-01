#pragma once
#include <sys/types.h>

/*
Tipos usados por todos os programas do trabalho.

Simulador: módulo principal. cria e encerra os processos e controla Ctrl+Z.
KernelSim: escalona, atende as chamadas e guarda os seis buffers.
InterController: envia IRQ0, IRQ1 e IRQ2.
Application: executa PC ate 5000 e troca dados com o parceiro.

As pipes reais levam mensagens entre esses programas.
Os tres pipes bidirecionais das aplicacoes sao simulados com seis vetores
de inteiros dentro do KernelSim (um vetor para cada sentido).

atoi: transforma um argumento de texto recebido por exec em numero.
snprintf: transforma um numero em texto para passa-lo por exec.
rand: escolhe aleatoriamente se a aplicacao solicita SEND ou RECV.
getenv e setenv: ativam opcoes usadas apenas nos testes.
sig_atomic_t: tipo seguro para as variaveis alteradas por sinais.
*/

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