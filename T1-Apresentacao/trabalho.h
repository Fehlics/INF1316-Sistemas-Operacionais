#pragma once
#include <sys/types.h>

/*
===============================================================================
MAPA NUMERADO DO FLUXO GERAL DO T1
===============================================================================

Use estes numeros para acompanhar os comentarios dos outros arquivos.

ETAPA 01 - Simulador prepara sinais e cria os canais de comunicacao.
ETAPA 02 - Simulador cria A1...A6; cada filho nasce parado com SIGSTOP.
ETAPA 03 - Simulador cria KernelSim e entrega pipes e PIDs das Applications.
ETAPA 04 - KernelSim monta os PCBs e libera a primeira Application por RR.
ETAPA 05 - Simulador cria InterController.
ETAPA 06 - Application incrementa PC e envia seu CONTEXTO ao KernelSim.
ETAPA 07 - Application pode pedir SEND ou RECV.
ETAPA 08 - KernelSim coloca a syscall na FIFO, bloqueia o processo e escalona.
ETAPA 09 - InterController gera IRQ0, IRQ1 e IRQ2.
ETAPA 10 - IRQ0 faz o KernelSim executar a troca do Round Robin.
ETAPA 11 - IRQ1/IRQ2 concluem RECV/SEND usando os buffers simulados.
ETAPA 12 - Application recebe a resposta, restaura PC/N e continua.
ETAPA 13 - Ctrl+Z pausa, mostra os PCBs e depois pode retomar a simulacao.
ETAPA 14 - Ao atingir o limite, Application envia FIM.
ETAPA 15 - KernelSim marca a Application como TERMINADO e confirma o fim.
ETAPA 16 - Simulador recolhe Applications encerradas usando waitpid.
ETAPA 17 - Ctrl+C encerra KernelSim, InterController e filhos restantes.

Observacao: Testes.c nao e uma nova etapa do fluxo normal. Ele apenas inicia o
mesmo sistema com configuracoes controladas para exercitar essas etapas.
===============================================================================
*/

/*
===============================================================================
trabalho.h
===============================================================================

Este arquivo reune as definicoes compartilhadas pelos quatro programas do
simulador. Ele funciona como o "vocabulário comum" do sistema.

Visao geral dos modulos:
- Simulador.c: cria todos os processos, controla Ctrl+Z/Ctrl+C e mostra estados.
- KernelSim.c: faz o Round Robin, controla bloqueios, filas e pipes simulados.
- InterController.c: gera as interrupcoes IRQ0, IRQ1 e IRQ2.
- Application.c: codigo executado por A1...A6.
- Testes.c: executa o Simulador em cenarios controlados.

Comunicacao:
- Existem pipes REAIS do Unix entre os programas acima.
- Os tres pipes bidirecionais pedidos no trabalho sao SIMULADOS pelo KernelSim
  usando seis buffers de inteiros: um buffer para cada sentido de comunicacao.

Funcoes auxiliares que aparecem no projeto:
- atoi: converte texto para int. Ex.: "3" -> 3.
- atol: converte texto para long, usado para recuperar PID.
- snprintf: monta texto dentro de um vetor de char.
- rand: gera numeros pseudoaleatorios.
- getenv/setenv: leem/criam variaveis de ambiente usadas nos testes.
- sig_atomic_t: tipo indicado para variaveis alteradas por tratadores de sinal.
===============================================================================
*/

/* Quantidade de aplicacoes e numero normal de iteracoes de cada uma. */
enum { TOTAL = 6, MAX = 5000 };

/*
Tipos de mensagem enviados pela pipe principal de controle.
IRQ       -> interrupcao enviada pelo InterController.
PEDIDO    -> pedido de SEND ou RECV feito por uma Application.
CONTEXTO  -> atualizacao de PC e N de uma Application.
FIM       -> aviso de que a Application terminou.
PAUSAR    -> inicia a pausa da simulacao.
MOSTRAR   -> pede a fotografia definitiva dos processos pausados.
RETOMAR   -> continua a execucao.
*/
enum { IRQ = 1, PEDIDO, CONTEXTO, FIM, PAUSAR, MOSTRAR, RETOMAR };

/* Operacao associada a syscall simulada ou a interrupcao correspondente. */
enum { NENHUMA, RECEBER, ENVIAR };

/*
Endereco LOGICO usado na syscall simulada:
SEND envia o PC; RECV recebe o valor em N.
Nao sao ponteiros reais entre processos diferentes.
*/
enum { SEM_ENDERECO, END_PC, END_N };

/* Estados usados no PCB simulado de cada Application. */
enum { PRONTO, EXECUTANDO, BLOQUEADO, TERMINADO };

/*
Mensagem enviada pela pipe principal.
Nem todos os campos sao usados em todos os tipos de mensagem.
*/
typedef struct {
    int tipo;
    int id;
    int op;
    int pc;
    int n;
    int endereco;
} Mensagem;

/*
Resposta do KernelSim para uma Application.
A resposta libera a Application que estava esperando uma syscall ou o termino.
*/
typedef struct {
    int op;
    int pc;
    int n;
} Resposta;

/*
PCB simplificado de uma Application.
Guarda exatamente as informacoes que o KernelSim precisa para escalonar,
mostrar o estado do processo e representar o contexto pedido no trabalho.
*/
typedef struct {
    pid_t pid;       /* PID real do processo Unix. */
    int pc;          /* Program Counter simulado. */
    int n;           /* Ultimo valor recebido do parceiro. */
    int estado;      /* PRONTO, EXECUTANDO, BLOQUEADO ou TERMINADO. */
    int op;          /* SEND/RECV pendente, se houver. */
    int endereco;    /* END_PC ou END_N durante uma syscall. */
    int leituras;    /* Quantas operacoes RECV ja foram concluidas. */
    int escritas;    /* Quantas operacoes SEND ja foram concluidas. */
} Processo;

/*
Fotografia enviada pelo KernelSim ao Simulador quando Ctrl+Z e usado.
"atual" guarda o indice (0..5) da Application que estava usando a CPU.
*/
typedef struct {
    int fase;
    int atual;
    Processo processos[TOTAL];
} Fotografia;
