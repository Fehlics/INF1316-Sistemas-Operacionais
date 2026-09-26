#include "testes.h"
#include "../trabalho.h"

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Teste de integracao Linux: confere filhos do Simulador pelo /proc. */

/* Encontra os filhos diretos atraves do PPid presente em /proc/PID/status. */
static int ler_filhos(pid_t pai, pid_t filhos[], int capacidade) {
    DIR *diretorio = opendir("/proc");
    if (!diretorio) return -1;
    struct dirent *entrada;
    int quantidade = 0;

    while ((entrada = readdir(diretorio)) != NULL) {
        char *fim;
        long pid = strtol(entrada->d_name, &fim, 10);
        if (!*entrada->d_name || *fim || pid <= 0) continue;
        char caminho[80], linha[160];
        snprintf(caminho, sizeof caminho, "/proc/%ld/status", pid);
        FILE *arquivo = fopen(caminho, "r");
        if (!arquivo) continue;
        long ppid = -1;
        while (fgets(linha, sizeof linha, arquivo)) {
            if (sscanf(linha, "PPid: %ld", &ppid) == 1) break;
        }
        fclose(arquivo);
        if (ppid == (long)pai) {
            if (quantidade == capacidade) {
                closedir(diretorio);
                return -1;
            }
            filhos[quantidade++] = (pid_t)pid;
        }
    }
    closedir(diretorio);
    return quantidade;
}

/* Le o nome do executavel do processo. */
static int ler_nome(pid_t pid, char nome[], size_t tamanho) {
    char caminho[80];
    snprintf(caminho, sizeof caminho, "/proc/%ld/comm", (long)pid);
    FILE *arquivo = fopen(caminho, "r");
    if (!arquivo) return 0;
    if (!fgets(nome, (int)tamanho, arquivo)) {
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);
    nome[strcspn(nome, "\n")] = '\0';
    return 1;
}

/* Le o ID que foi passado como segundo argumento para Application. */
static int ler_id_aplicacao(pid_t pid) {
    char caminho[80];
    snprintf(caminho, sizeof caminho, "/proc/%ld/cmdline", (long)pid);
    FILE *arquivo = fopen(caminho, "rb");
    if (!arquivo) return 0;
    int caractere;
    do { caractere = fgetc(arquivo); } while (caractere != EOF && caractere != 0);
    char argumento[16];
    int tamanho = 0;
    if (caractere != EOF) {
        while ((caractere = fgetc(arquivo)) != EOF && caractere != 0 &&
               tamanho < (int)sizeof argumento - 1) {
            argumento[tamanho++] = (char)caractere;
        }
    }
    fclose(arquivo);
    if (caractere != 0 || !tamanho) return 0;
    argumento[tamanho] = '\0';
    char *fim;
    long id = strtol(argumento, &fim, 10);
    return (*fim || id < 1 || id > QUANTIDADE_APLICACOES) ? 0 : (int)id;
}

/* Confere que ha exatamente A1...A6, um KernelSim e um InterController. */
static int conferir_filhos(pid_t filhos[], int quantidade) {
    int aplicacoes = 0, kernels = 0, controladores = 0;
    int ids[QUANTIDADE_APLICACOES] = {0};
    if (quantidade != QUANTIDADE_APLICACOES + 2) return 0;

    for (int i = 0; i < quantidade; i++) {
        char nome[40];
        if (!ler_nome(filhos[i], nome, sizeof nome)) return 0;
        if (strcmp(nome, "Application") == 0) {
            int id = ler_id_aplicacao(filhos[i]);
            if (!id || ids[id - 1]) return 0;
            ids[id - 1] = 1;
            aplicacoes++;
        } else if (strcmp(nome, "KernelSim") == 0) {
            kernels++;
        } else if (strcmp(nome, "InterController") == 0) {
            controladores++;
        }
    }
    return aplicacoes == QUANTIDADE_APLICACOES && kernels == 1 && controladores == 1;
}

/* Aguarda que os PIDs registrados deixem de existir. */
static int filhos_encerrados(pid_t filhos[], int quantidade) {
    for (int tentativa = 0; tentativa < 30; tentativa++) {
        int restantes = 0;
        for (int i = 0; i < quantidade; i++) {
            if (kill(filhos[i], 0) == 0 || errno != ESRCH) restantes++;
        }
        if (!restantes) return 1;
        usleep(100000);
    }
    return 0;
}

int testar_processos(void) {
    const char *executaveis[] = {
        "./Simulador", "./KernelSim", "./InterController", "./Application"
    };
    pid_t filhos[QUANTIDADE_APLICACOES + 2] = {0};
    int quantidade = 0, criacao_correta = 0, terminou = 0, status = 0;

    puts("\n[TesteProcessos] Verificando executaveis...");
    for (int i = 0; i < 4; i++) {
        if (access(executaveis[i], X_OK) != 0) {
            printf("[ERRO] Compile e execute a partir de T1: %s\n", executaveis[i]);
            return 0;
        }
    }

    pid_t simulador = fork();
    if (simulador == -1) {
        perror("fork teste");
        return 0;
    }
    if (simulador == 0) {
        /* Um grupo proprio permite encerrar os processos do teste em caso de erro. */
        setpgid(0, 0);
        FILE *saida = fopen("/dev/null", "w");
        if (saida) {
            dup2(fileno(saida), STDOUT_FILENO);
            dup2(fileno(saida), STDERR_FILENO);
            fclose(saida);
        }
        execl("./Simulador", "Simulador", (char *)NULL);
        _exit(127);
    }

    puts("[TesteProcessos] Aguardando os 8 filhos do Simulador...");
    /* Tempo para os seis processos passarem pelo escalonador e executarem exec. */
    for (int tentativa = 0; tentativa < 120; tentativa++) {
        pid_t resultado = waitpid(simulador, &status, WNOHANG);
        if (resultado == simulador) { terminou = 1; break; }
        if (resultado == -1) break;
        quantidade = ler_filhos(simulador, filhos, QUANTIDADE_APLICACOES + 2);
        if (conferir_filhos(filhos, quantidade)) {
            criacao_correta = 1;
            break;
        }
        usleep(100000);
    }

    if (criacao_correta) {
        puts("[OK] Encontrados: A1 a A6, KernelSim e InterController.");
    } else {
        puts("[ERRO] Nao foram encontrados os 8 processos esperados.");
    }

    if (!terminou) kill(simulador, SIGINT);
    int encerramento_correto = 0;
    if (terminou) {
        puts("[ERRO] Simulador encerrou antes do pedido.");
    } else {
        for (int tentativa = 0; tentativa < 50; tentativa++) {
            pid_t resultado = waitpid(simulador, &status, WNOHANG);
            if (resultado == simulador) {
                terminou = 1;
                encerramento_correto = WIFEXITED(status) && WEXITSTATUS(status) == 130;
                break;
            }
            if (resultado == -1) break;
            usleep(100000);
        }
    }

    if (!terminou) {
        kill(-simulador, SIGKILL);
        kill(simulador, SIGKILL);
        waitpid(simulador, NULL, 0);
    }
    int limpeza_correta = criacao_correta && filhos_encerrados(filhos, quantidade);
    if (encerramento_correto && limpeza_correta) {
        puts("[OK] Simulador encerrado sem deixar os 8 filhos ativos.");
    } else {
        puts("[ERRO] Falha no encerramento ou limpeza dos processos.");
    }
    return criacao_correta && encerramento_correto && limpeza_correta;
}
