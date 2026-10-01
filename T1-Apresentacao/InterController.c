#include "trabalho.h"
#include <stdlib.h>
#include <unistd.h>

/*
===============================================================================
InterController.c
===============================================================================

Representa o controlador de interrupcoes do trabalho.

A cada ciclo:
- sempre envia IRQ0, indicando fim do quantum do Round Robin;
- com 10% de chance envia IRQ1, indicando fim de uma leitura (RECV);
- com 5% de chance envia IRQ2, indicando fim de uma escrita (SEND).

Na implementacao:
- m.op = NENHUMA  -> IRQ0
- m.op = RECEBER  -> IRQ1
- m.op = ENVIAR   -> IRQ2

O processo roda em loop ate a pipe ser fechada pelo restante do sistema.
===============================================================================
*/

int main(int argc, char *argv[]) {
    /*
    Recebe apenas um argumento: o descritor da pipe principal de controle.
    E por essa pipe que as interrupcoes chegam ao KernelSim.
    */
    if (argc != 2)
        return 1;

    int controle = atoi(argv[1]);

    /*
    [ETAPA 09] GERACAO DAS INTERRUPCOES
    Este loop e o ponto exato em que surgem IRQ0, IRQ1 e IRQ2.
    - IRQ0: sempre, depois do sleep.
    - IRQ1: RECEBER, normalmente com 10% de chance.
    - IRQ2: ENVIAR, normalmente com 5% de chance.
    */
    while (1) {
        /*
        Espera entre duas rodadas de interrupcoes.
        Foi adotado 1 segundo para usar apenas sleep().
        */
        sleep(1);

        /* IRQ0: sempre ocorre e provoca uma nova decisao de escalonamento. */
        Mensagem m = {0};
        m.tipo = IRQ;
        m.op = NENHUMA;

        if (write(controle, &m, sizeof m) != sizeof m)
            break;

        /*
        IRQ1: termina o primeiro RECV da fila correspondente.
        Nos testes, TESTE_AUTO faz a interrupcao acontecer em todo ciclo
        para o teste nao depender de sorte.
        */
        if (getenv("TESTE_AUTO") != NULL || rand() % 100 < 10) {
            m.op = RECEBER;

            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }

        /*
        IRQ2: termina o primeiro SEND da fila correspondente.
        A probabilidade normal e menor que a de leitura: 5%.
        */
        if (getenv("TESTE_AUTO") != NULL || rand() % 100 < 5) {
            m.op = ENVIAR;

            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }
    }

    close(controle);
    return 0;
}
