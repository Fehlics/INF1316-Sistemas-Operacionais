#include "testes.h"
#include "../trabalho.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Confere as mensagens do Simulador em vez de examinar /proc. */
int testar_processos(void) {
    const char *nomes[] = {
        "./Simulador", "./KernelSim", "./InterController", "./Application"
    };
    puts("\n[TesteProcessos] Verificando os quatro executaveis...");
    for (int i = 0; i < 4; i++) {
        if (access(nomes[i], X_OK) != 0) {
            printf("[ERRO] Compile o executavel %s.\n", nomes[i]);
            return 0;
        }
    }
    int log[2];
    if (pipe(log) != 0) return 0;
    pid_t sim = fork();
    if (sim < 0) { close(log[0]); close(log[1]); return 0; }
    if (sim == 0) {
        close(log[0]);
        dup2(log[1], STDOUT_FILENO);
        close(log[1]);
        execl("./Simulador", "Simulador", (char *)NULL);
        _exit(127);
    }
    close(log[1]);
    /* Tempo suficiente para o Simulador criar seus oito filhos. */
    sleep(2);
    kill(sim, SIGINT);

    int status = 0, terminou = 0;
    for (int i = 0; i < 5; i++) {
        pid_t r = waitpid(sim, &status, WNOHANG);
        if (r == sim) { terminou = 1; break; }
        if (r < 0) break;
        sleep(1);
    }
    if (!terminou) {
        kill(sim, SIGTERM);
        waitpid(sim, NULL, 0);
        close(log[0]);
        return 0;
    }

    char texto[8192];
    size_t total = 0;
    ssize_t n;
    while (total < sizeof texto - 1 &&
           (n = read(log[0], texto + total, sizeof texto - total - 1)) > 0)
        total += (size_t)n;
    close(log[0]);
    texto[total] = '\0';
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 130 ||
        !strstr(texto, "[Simulador] Processos iniciados.") ||
        !strstr(texto, "[Simulador] Encerrado.")) return 0;

    pid_t filhos[QUANTIDADE_APLICACOES + 2] = {0};
    for (int id = 1; id <= QUANTIDADE_APLICACOES; id++) {
        char padrao[50];
        snprintf(padrao, sizeof padrao, "[Simulador] Criou A%d PID=", id);
        char *p = strstr(texto, padrao);
        if (!p) return 0;
        filhos[id - 1] = (pid_t)atol(p + strlen(padrao));
    }
    char *k = strstr(texto, "[Simulador] Criou KernelSim PID=");
    char *c = strstr(texto, "[Simulador] Criou InterController PID=");
    if (!k || !c) return 0;
    filhos[6] = (pid_t)atol(k + strlen("[Simulador] Criou KernelSim PID="));
    filhos[7] = (pid_t)atol(c + strlen("[Simulador] Criou InterController PID="));
    for (int i = 0; i < 8; i++) {
        if (filhos[i] <= 0 || kill(filhos[i], 0) != -1 || errno != ESRCH)
            return 0;
    }
    puts("[OK] Os seis processos, o KernelSim e o controlador foram criados.");
    puts("[OK] O encerramento recolheu todos os oito filhos.");
    return 1;
}
