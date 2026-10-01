#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
===============================================================================
Application.c
===============================================================================

Este programa e executado seis vezes: uma vez para A1, A2, ..., A6.

Cada Application:
1. Mantem duas variaveis principais: PC e N.
2. Informa periodicamente PC/N ao KernelSim.
3. Pode solicitar SEND ou RECV.
4. Quando pede uma syscall, espera a resposta do KernelSim.
5. Ao chegar ao limite de iteracoes, avisa que terminou e espera confirmacao.

Importante para explicar:
- A Application NAO escolhe quando entra ou sai da CPU.
- Quem controla isso e o KernelSim, usando SIGSTOP e SIGCONT.
- A Application apenas executa normalmente quando esta liberada.
===============================================================================
*/

int main(int argc, char *argv[]) {
    /*
    O Simulador inicia cada Application passando:
    argv[1] = id da aplicacao (1..6)
    argv[2] = descritor da pipe de controle
    argv[3] = descritor da pipe de resposta exclusiva daquela aplicacao
    */
    if (argc != 4)
        return 1;

    int id = atoi(argv[1]);
    int controle = atoi(argv[2]);
    int resposta = atoi(argv[3]);

    /* Contexto principal da aplicacao. */
    int pc = 0;
    int n = 0;

    /* Na execucao normal o limite e MAX = 5000. */
    int limite = MAX;

    /*
    "modo" existe apenas para os testes.
    Na execucao normal ele vale 0 e SEND/RECV sao escolhidos aleatoriamente.
    */
    int modo = 0;

    if (getenv("TESTE_MAX") != NULL)
        limite = atoi(getenv("TESTE_MAX"));

    if (getenv("TESTE_OP") != NULL)
        modo = atoi(getenv("TESTE_OP"));

    /*
    Loop principal da aplicacao.
    O PC cresce ate atingir o numero de iteracoes desejado.
    */
    while (pc < limite) {
        /*
        [ETAPA 06] EXECUCAO NORMAL DA APPLICATION
        Cada volta representa uma nova iteracao. O PC e incrementado e logo
        abaixo o novo PC/N e enviado ao KernelSim como mensagem CONTEXTO.
        */
        pc++;

        /*
        Atualiza o KernelSim com o contexto atual.
        Assim, o PCB do kernel sabe os valores mais recentes de PC e N.
        */
        Mensagem m = {0};
        m.tipo = CONTEXTO;
        m.id = id;
        m.pc = pc;
        m.n = n;

        if (write(controle, &m, sizeof m) != sizeof m)
            return 1;

        /*
        Execucao normal: uma espera de 1 segundo por iteracao.
        TESTE_RAPIDO remove a maioria dessas esperas para os testes acabarem
        em poucos segundos.
        */
        if (getenv("TESTE_RAPIDO") == NULL)
            sleep(1);
        else if (id == 1 && (pc == 1 || pc == limite / 2))
            sleep(1);

        /*
        Decide se havera syscall nesta iteracao.
        NENHUMA significa continuar sem comunicacao.
        */
        int op = NENHUMA;

        /*
        Nos testes, o modo força operacoes previsiveis no PC=2:
        modo 1 -> impares enviam, pares recebem
        modo 2 -> pares enviam, impares recebem
        modo 3 -> pares tentam receber sem envio anterior
        */
        if (modo > 0 && pc == 2) {
            if (modo == 1 && id % 2 == 1)
                op = ENVIAR;

            if (modo == 1 && id % 2 == 0)
                op = RECEBER;

            if (modo == 2 && id % 2 == 0)
                op = ENVIAR;

            if (modo == 2 && id % 2 == 1)
                op = RECEBER;

            if (modo == 3 && id % 2 == 0)
                op = RECEBER;
        }
        else if (modo == 0 && getenv("TESTE_RAPIDO") == NULL) {
            /*
            Execucao normal: 15% de chance de ocorrer uma syscall.
            Depois, escolhe aleatoriamente SEND ou RECV.
            */
            if (rand() % 100 < 15) {
                if (rand() % 2 == 0)
                    op = ENVIAR;
                else
                    op = RECEBER;
            }
        }

        /*
        [ETAPA 07] PEDIDO DE SEND OU RECV
        Se "op" deixou de ser NENHUMA, a Application transforma a mensagem em
        PEDIDO e informa END_PC para SEND ou END_N para RECV.
        */
        if (op != NENHUMA) {
            /*
            Envia o pedido de syscall ao KernelSim.
            SEND trabalha com PC; RECV trabalha com N.
            */
            m.tipo = PEDIDO;
            m.op = op;

            if (op == ENVIAR)
                m.endereco = END_PC;
            else
                m.endereco = END_N;

            if (write(controle, &m, sizeof m) != sizeof m)
                return 1;

            /*
            A leitura abaixo bloqueia a Application ate o KernelSim concluir
            a operacao. Isso representa a espera pela E/S simulada.
            */
            /*
            [ETAPA 12] ESPERA E RETORNO DA SYSCALL
            A Application fica nesta leitura ate o KernelSim concluir a
            operacao. A resposta devolve o contexto PC/N para a continuacao.
            */
            Resposta r;
            if (read(resposta, &r, sizeof r) != sizeof r)
                return 1;

            /* Confere se a resposta pertence a operacao esperada. */
            if (r.op != op || r.pc != pc)
                return 1;

            /*
            No teste de buffer vazio, um RECV deve retornar N = 0.
            */
            if (modo == 3 && r.n != 0)
                return 1;

            /*
            Restaura o contexto devolvido pelo KernelSim.
            RECV pode alterar N; PC continua representando a mesma iteracao.
            */
            pc = r.pc;
            n = r.n;

            /* Informa ao KernelSim o contexto atualizado apos a syscall. */
            m.tipo = CONTEXTO;
            m.pc = pc;
            m.n = n;

            if (write(controle, &m, sizeof m) != sizeof m)
                return 1;
        }
    }

    /*
    Depois da ultima iteracao, avisa ao KernelSim que terminou.
    A Application so encerra depois de receber a confirmacao.
    */
    /*
    [ETAPA 14] FIM DA APPLICATION
    Ao sair do while, PC atingiu o limite (5000 na execucao normal).
    A Application envia FIM e espera a confirmacao do KernelSim antes de sair.
    */
    Mensagem fim = {0};
    fim.tipo = FIM;
    fim.id = id;
    fim.pc = pc;
    fim.n = n;

    if (write(controle, &fim, sizeof fim) != sizeof fim)
        return 1;

    Resposta r;
    if (read(resposta, &r, sizeof r) != sizeof r || r.op != NENHUMA)
        return 1;

    printf("FINAL A%d PC=%d N=%d\n", id, pc, n);

    close(controle);
    close(resposta);
    return 0;
}
