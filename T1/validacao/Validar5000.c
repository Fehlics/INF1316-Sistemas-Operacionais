#include "../trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    const char *nome;
    int syscalls, pausa;
} Cenario;

typedef struct {
    int finalizou[QUANTIDADE_APLICACOES];
    int auditou[QUANTIDADE_APLICACOES];
    int terminou[QUANTIDADE_APLICACOES];
    int confirmou[QUANTIDADE_APLICACOES];
    int recolheu[QUANTIDADE_APLICACOES];
    int estado[QUANTIDADE_APLICACOES];
    int repetidos[QUANTIDADE_APLICACOES];
    int escrita_a1, leitura_a2, envio, recebimento;
    int escalonamentos, pausa, retomada, kernel_final, sim_final;
} Contagem;

static int analisar(FILE *arquivo, Cenario c) {
    Contagem t = {0};
    char linha[512], resultado[20];
    int id, pc, n, leituras, escritas, passos, repetidos, erros;
    long pid;
    int invalido = 0;
    rewind(arquivo);
    while (fgets(linha, sizeof linha, arquivo)) {
        if (sscanf(linha, "[A%d] Finalizada (PC=%d, N=%d).", &id, &pc, &n) == 3) {
            if (id >= 1 && id <= 6 && pc == MAX_ITERACOES)
                t.finalizou[id - 1]++;
            else invalido++;
        } else if (sscanf(linha, "[Validacao5000] A%d %19s passos=%d repetidos=%d erros=%d PC=%d",
                          &id, resultado, &passos, &repetidos, &erros, &pc) == 6) {
            if (id >= 1 && id <= 6 && !strcmp(resultado, "PASSOU") &&
                passos == MAX_ITERACOES && pc == MAX_ITERACOES && !erros) {
                t.auditou[id - 1]++;
                t.repetidos[id - 1] = repetidos;
            } else invalido++;
        } else if (sscanf(linha,
                   "[Kernel] A%d TERMINADO (PC=%d, N=%d, leituras=%d, escritas=%d).",
                   &id, &pc, &n, &leituras, &escritas) == 5) {
            if (id >= 1 && id <= 6 && pc == MAX_ITERACOES) {
                t.terminou[id - 1]++;
                if (id == 1) t.escrita_a1 = escritas;
                if (id == 2) t.leitura_a2 = leituras;
            } else invalido++;
        } else if (sscanf(linha, "[Simulador] Recolheu A%d (PID=%ld,", &id, &pid) == 2 &&
                   strstr(linha, "termino normal")) {
            if (id >= 1 && id <= 6 && pid > 0) t.recolheu[id - 1]++;
            else invalido++;
        } else if (strstr(linha, "Termino confirmado pelo KernelSim.") &&
                   sscanf(linha, "[A%d]", &id) == 1) {
            if (id >= 1 && id <= 6) t.confirmou[id - 1]++;
            else invalido++;
        } else if (sscanf(linha, "[Estado] A%d ", &id) == 1 && id >= 1 && id <= 6)
            t.estado[id - 1]++;

        if (strstr(linha, "[Kernel] Executando A")) t.escalonamentos++;
        if (strstr(linha, "[Kernel] A1 enviou PC=2 para A2.")) t.envio++;
        if (strstr(linha, "[Kernel] A2 recebeu N=")) t.recebimento++;
        if (strstr(linha, "ESTADO DOS PROCESSOS (PAUSADO)")) t.pausa++;
        if (strstr(linha, "[Simulador] Execucao retomada.")) t.retomada++;
        if (strstr(linha, "[Kernel] Todas as seis aplicacoes terminaram.")) t.kernel_final++;
        if (strstr(linha, "[Simulador] As seis aplicacoes foram recolhidas.")) t.sim_final++;
    }
    int ok = !invalido && t.kernel_final == 1 && t.sim_final == 1 &&
             t.escalonamentos > QUANTIDADE_APLICACOES;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        int esperado = c.syscalls && i < 2 ? 1 : 0;
        if (t.finalizou[i] != 1 || t.auditou[i] != 1 || t.terminou[i] != 1 ||
            t.confirmou[i] != 1 || t.recolheu[i] != 1 ||
            t.repetidos[i] != esperado || (c.pausa && t.estado[i] != 1)) {
            printf("[ERRO] A%d: fim=%d audit=%d kernel=%d conf=%d recol=%d rep=%d estado=%d\n",
                   i + 1, t.finalizou[i], t.auditou[i], t.terminou[i],
                   t.confirmou[i], t.recolheu[i], t.repetidos[i], t.estado[i]);
            ok = 0;
        }
    }
    if (c.syscalls && (!t.envio || !t.recebimento ||
                       !t.escrita_a1 || !t.leitura_a2)) ok = 0;
    if (c.pausa && (t.pausa != 1 || t.retomada != 1)) ok = 0;
    if (ok) {
        printf("[OK] %s: 6 x 5000 = 30000 iteracoes, %d escalonamentos.",
               c.nome, t.escalonamentos);
        if (c.syscalls) printf(" SEND/RECV concluidos.");
        if (c.pausa) printf(" Ctrl+Z pausou e retomou.");
        putchar('\n');
    } else printf("[ERRO] %s: auditoria incompleta (%d erros).\n", c.nome, invalido);
    return ok;
}

static int executar(Cenario c) {
    FILE *registro = tmpfile();
    if (!registro) return 0;
    printf("[Validacao5000] Iniciando: %s...\n", c.nome);
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) { fclose(registro); return 0; }
    if (pid == 0) {
        if (dup2(fileno(registro), STDOUT_FILENO) < 0 ||
            dup2(fileno(registro), STDERR_FILENO) < 0) _exit(127);
        fclose(registro);
        unsetenv("TESTE_MAX_ITERACOES");
        unsetenv("TESTE_SYSCALL");
        unsetenv("TESTE_SEM_SYSCALL");
        setenv("TESTE_INTERVALO_US", "500", 1);
        setenv("TESTE_INTERVALO_IRQ_US", "50000", 1);
        setenv("TESTE_VALIDAR_PC", "1", 1);
        setenv("TESTE_ENCERRAR_AO_FINAL", "1", 1);
        setenv(c.syscalls ? "TESTE_SYSCALL" : "TESTE_SEM_SYSCALL", "1", 1);
        execl("./Simulador", "Simulador", (char *)NULL);
        _exit(127);
    }

    int status = 0, terminou = 0;
    for (int ciclo = 0; ciclo < 900; ciclo++) {
        pid_t retorno = waitpid(pid, &status, WNOHANG);
        if (retorno == pid) { terminou = 1; break; }
        if (retorno < 0) break;
        if (c.pausa && (ciclo == 10 || ciclo == 40)) kill(pid, SIGTSTP);
        usleep(100000);
    }
    if (!terminou) {
        kill(pid, SIGINT);
        for (int i = 0; i < 50; i++) {
            pid_t retorno = waitpid(pid, &status, WNOHANG);
            if (retorno == pid) { terminou = 1; break; }
            if (retorno < 0) break;
            usleep(100000);
        }
        if (!terminou) { kill(pid, SIGTERM); waitpid(pid, &status, 0); }
        puts("[ERRO] Tempo limite excedido.");
        fclose(registro);
        return 0;
    }
    int ok = WIFEXITED(status) && WEXITSTATUS(status) == 0 && analisar(registro, c);
    fclose(registro);
    return ok;
}

int main(int argc, char *argv[]) {
    Cenario cenarios[] = {
        {"5000 iteracoes sem syscall", 0, 0},
        {"5000 iteracoes com SEND e RECV", 1, 0},
        {"5000 iteracoes com pausa e retomada", 0, 1}
    };
    int inicio = 0, fim = 3;
    if (argc == 2) {
        int escolhido = atoi(argv[1]);
        if (escolhido < 1 || escolhido > 3) {
            puts("Uso: ./Validar5000 [1|2|3]");
            return 1;
        }
        inicio = escolhido - 1;
        fim = escolhido;
    } else if (argc != 1) return 1;

    int passou = 0;
    puts("=== VALIDACAO PROLONGADA DAS 5000 ITERACOES ===");
    for (int i = inicio; i < fim; i++) passou += executar(cenarios[i]);
    printf("Resultado: %d de %d cenarios passaram.\n", passou, fim - inicio);
    return passou == fim - inicio ? 0 : 1;
}
