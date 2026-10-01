#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
===============================================================================
Testes.c
===============================================================================

Os testes sao de integracao: eles executam o proprio ./Simulador.

Nao tentam testar cada funcao isoladamente. A ideia e verificar se o sistema
completo consegue executar e terminar normalmente em alguns cenarios simples.

Variaveis de ambiente usadas:
- TESTE_MAX: troca temporariamente o numero de iteracoes.
- TESTE_OP: escolhe um padrao fixo de SEND/RECV.
- TESTE_RAPIDO: remove a maior parte das esperas de 1 segundo.
- TESTE_AUTO: faz o InterController gerar IRQ1/IRQ2 em todo ciclo e permite
  que o Simulador encerre sozinho quando todas as Applications terminarem.
===============================================================================
*/

/*
Executa um cenario de teste.

maximo     -> numero de iteracoes de cada Application.
modo       -> padrao de comunicacao definido em Application.c.
com_pausa  -> se vale 1, o teste tambem envia dois Ctrl+Z simulados.
*/
/*
APOIO A TODAS AS ETAPAS - TESTES DE INTEGRACAO
Testes.c nao altera a ordem do fluxo numerado. Ele cria uma execucao real do
Simulador e usa variaveis de ambiente para tornar SEND/RECV e IRQs previsiveis.
Assim, os testes percorrem as mesmas ETAPAS 01 a 17 do sistema normal.
*/
int executar_teste(int maximo, int modo, int com_pausa) {
    pid_t pid = fork();
    if (pid < 0)
        return 0;

    if (pid == 0) {
        /*
        O filho configura o cenario e depois e substituido por ./Simulador.
        */
        char limite[16], operacao[8];
        snprintf(limite, sizeof limite, "%d", maximo);
        snprintf(operacao, sizeof operacao, "%d", modo);
        setenv("TESTE_MAX", limite, 1);
        setenv("TESTE_OP", operacao, 1);
        setenv("TESTE_RAPIDO", "1", 1);
        setenv("TESTE_AUTO", "1", 1);
        execl("./Simulador", "Simulador", (char *)NULL);
        exit(1);
    }

    /*
    Quando solicitado, o processo pai envia SIGTSTP ao Simulador.
    O primeiro pausa e o segundo retoma.
    */
    if (com_pausa) {
        sleep(1);
        kill(pid, SIGTSTP);
        sleep(2);
        kill(pid, SIGTSTP);
    }

    /*
    Espera o Simulador terminar.
    O teste passa quando o processo encerra normalmente com codigo 0.
    */
    int status;
    if (waitpid(pid, &status, 0) != pid)
        return 0;
    if (!WIFEXITED(status))
        return 0;
    return WEXITSTATUS(status) == 0;
}

/* Executa a bateria curta ou, com argumento 5000, o teste prolongado. */
int main(int argc, char *argv[]) {
    /*
    ./Testes 5000:
    seis Applications x 5000 iteracoes, com comunicacao e pausa/retomada.
    */
    if (argc == 2 && strcmp(argv[1], "5000") == 0) {
        puts("TESTE: 5000 iteracoes, comunicacao e Ctrl+Z");
        if (executar_teste(MAX, 1, 1)) {
            puts("PASSOU: seis aplicacoes encerraram normalmente.");
            return 0;
        }
        puts("FALHOU");
        return 1;
    }

    /* ./Testes sem argumentos: cinco cenarios rapidos. */
    int aprovados = 0;
    puts("TESTE 1: sem comunicacao");
    aprovados += executar_teste(20, 0, 0);
    puts("TESTE 2: tres pares, sentido ida");
    aprovados += executar_teste(20, 1, 0);
    puts("TESTE 3: tres pares, sentido volta");
    aprovados += executar_teste(20, 2, 0);
    puts("TESTE 4: leitura dos buffers vazios");
    aprovados += executar_teste(20, 3, 0);
    puts("TESTE 5: pausa e retomada");
    aprovados += executar_teste(100, 0, 1);
    printf("Resultado: %d de 5 testes passaram.\n", aprovados);
    return aprovados == 5 ? 0 : 1;
}