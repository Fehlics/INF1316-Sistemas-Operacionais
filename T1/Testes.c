#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int executar_teste(int maximo, int modo, int com_pausa) {
    pid_t pid = fork();
    if (pid < 0)
        return 0;

    if (pid == 0) {
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

    if (com_pausa) {
        sleep(1);
        kill(pid, SIGTSTP);
        sleep(2);
        kill(pid, SIGTSTP);
    }

    int status;
    if (waitpid(pid, &status, 0) != pid)
        return 0;
    if (!WIFEXITED(status))
        return 0;
    return WEXITSTATUS(status) == 0;
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "5000") == 0) {
        puts("TESTE: 5000 iteracoes, comunicacao e Ctrl+Z");
        if (executar_teste(MAX, 1, 1)) {
            puts("PASSOU: seis aplicacoes encerraram normalmente.");
            return 0;
        }
        puts("FALHOU");
        return 1;
    }

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