#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Teste integrado: inicia o Simulador, registra e verifica sua saida. */
static int testar(const char *nome, int maximo, int modo, int pausa) {
    FILE *log = tmpfile();
    if (!log)
        return 0;

    printf("[Teste] %s...\n", nome);
    fflush(stdout);
    pid_t filho = fork();
    if (filho < 0) {
        fclose(log);
        return 0;
    }
    if (filho == 0) {
        dup2(fileno(log), STDOUT_FILENO);
        dup2(fileno(log), STDERR_FILENO);
        fclose(log);

        char valor[16], operacao[8];
        snprintf(valor, sizeof valor, "%d", maximo);
        snprintf(operacao, sizeof operacao, "%d", modo);
        setenv("TESTE_MAX", valor, 1);
        setenv("TESTE_RAPIDO", "1", 1);
        setenv("TESTE_OP", operacao, 1);
        if (!modo)
            setenv("TESTE_SEM_OP", "1", 1);
        else
            unsetenv("TESTE_SEM_OP");
        setenv("TESTE_AUTO", "1", 1);
        setenv("TESTE_AUDIT", "1", 1);
        execl("./Simulador", "Simulador", (char *)NULL);
        exit(1);
    }

    int status = 0, saiu = 0;
    int limite = maximo == MAX ? 100 : 50;
    for (int i = 0; i < limite; i++) {
        pid_t resultado = waitpid(filho, &status, WNOHANG);
        if (resultado == filho) {
            saiu = 1;
            break;
        }
        if (resultado < 0)
            break;
        if (pausa && (i == 1 || i == 4))
            kill(filho, SIGTSTP);
        sleep(1);
    }

    if (!saiu) {
        kill(filho, SIGINT);
        /* Da tempo para o Simulador recolher os filhos antes de desistir. */
        for (int i = 0; i < 5; i++) {
            if (waitpid(filho, &status, WNOHANG) == filho) {
                saiu = 1;
                break;
            }
            sleep(1);
        }
        if (!saiu) {
            kill(filho, SIGKILL);
            waitpid(filho, &status, 0);
        }
    }

    int fim[TOTAL] = {0}, audit[TOTAL] = {0}, reconheceu[TOTAL] = {0};
    int recolheu[TOTAL] = {0}, enviado[TOTAL] = {0}, recebido[TOTAL] = {0};
    int estados[TOTAL] = {0}, retomou = 0, cpu = 0, erros = 0;
    char linha[512];
    rewind(log);
    while (fgets(linha, sizeof linha, log)) {
        int id, pc, n, l, e, passos, falhas;
        if (sscanf(linha, "FINAL A%d PC=%d N=%d", &id, &pc, &n) == 3) {
            if (id >= 1 && id <= TOTAL && pc == maximo)
                fim[id - 1]++;
            else
                erros++;
        } else if (sscanf(linha, "TERMINOU A%d PC=%d N=%d L=%d E=%d",
                          &id, &pc, &n, &l, &e) == 5) {
            if (id >= 1 && id <= TOTAL && pc == maximo)
                reconheceu[id - 1]++;
            else
                erros++;
        } else if (sscanf(linha, "AUDIT A%d CONT=%d ERROS=%d",
                          &id, &passos, &falhas) == 3) {
            if (id >= 1 && id <= TOTAL && passos == maximo && !falhas)
                audit[id - 1]++;
            else
                erros++;
        } else if (sscanf(linha, "RECOLHEU A%d", &id) == 1 &&
                   id >= 1 && id <= TOTAL) {
            recolheu[id - 1]++;
        } else if (sscanf(linha, "SEND A%d PC=%d", &id, &pc) == 2 &&
                   id >= 1 && id <= TOTAL) {
            enviado[id - 1]++;
        } else if (sscanf(linha, "RECV A%d N=%d", &id, &n) == 2 &&
                   id >= 1 && id <= TOTAL) {
            recebido[id - 1]++;
            if (modo == 4 && n != 0)
                erros++;
        } else if (sscanf(linha, "[Estado] A%d", &id) == 1 &&
                   id >= 1 && id <= TOTAL) {
            estados[id - 1]++;
        }

        if (strstr(linha, "RETOMOU"))
            retomou++;
        if (strncmp(linha, "CPU A", 5) == 0)
            cpu++;
    }
    fclose(log);

    int ok = saiu && WIFEXITED(status) && !WEXITSTATUS(status) &&
             !erros && cpu >= TOTAL;
    for (int i = 0; i < TOTAL; i++) {
        if (fim[i] != 1 || audit[i] != 1 || reconheceu[i] != 1 ||
            recolheu[i] != 1) {
            ok = 0;
        }

        int envia = modo == 2 ? i % 2 == 0 :
                    modo == 3 ? i % 2 != 0 : modo == 1 && i == 0;
        int recebe = modo == 2 ? i % 2 != 0 :
                     modo == 3 ? i % 2 == 0 :
                     modo == 4 ? i % 2 != 0 : modo == 1 && i == 1;
        if (enviado[i] != envia || recebido[i] != recebe)
            ok = 0;
        if (pausa && estados[i] != 1)
            ok = 0;
    }
    if (pausa && retomou != 1)
        ok = 0;

    printf("[%s] %s: %d iteracoes (6 processos)%s\n",
           ok ? "OK" : "FALHOU", nome, maximo,
           pausa ? ", pausa/retomada" : "");
    return ok;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "5000") == 0) {
        int ok = testar("5000 iteracoes com comunicacao e Ctrl+Z", MAX, 2, 1);
        return ok ? 0 : 1;
    }

    int aprovados = 0;
    aprovados += testar("sem syscalls", 20, 0, 0);
    aprovados += testar("tres pares: ida", 20, 2, 0);
    aprovados += testar("tres pares: volta", 20, 3, 0);
    aprovados += testar("leitura com pipe vazio", 20, 4, 0);
    aprovados += testar("Ctrl+Z e retomada", 200, 0, 1);
    printf("Resultado: %d de 5 testes passaram.\n", aprovados);
    return aprovados == 5 ? 0 : 1;
}
