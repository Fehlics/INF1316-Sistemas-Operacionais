#include "../trabalho.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Teste prolongado separado de Testes.c: cada caso executa as seis
 * Applications REAIS ate MAX_ITERACOES, com um atraso reduzido APENAS
 * para teste. Nenhum processo recebe uma quantidade menor de iteracoes. */
typedef struct {
    const char *nome;
    int com_syscalls;
    int com_pausa;
} Cenario;

typedef struct {
    int aplicacao[QUANTIDADE_APLICACOES];
    int confirmacao[QUANTIDADE_APLICACOES];
    int kernel[QUANTIDADE_APLICACOES];
    int recolhido[QUANTIDADE_APLICACOES];
    int auditoria[QUANTIDADE_APLICACOES];
    int enviados;
    int recebidos;
    int escalonamentos;
    int snapshots[QUANTIDADE_APLICACOES];
    int pausas;
    int retomadas;
    int kernel_final;
    int simulador_final;
    int erros;
    int repetidos[QUANTIDADE_APLICACOES];
    int leituras[QUANTIDADE_APLICACOES];
    int escritas[QUANTIDADE_APLICACOES];
} Resultado;

/* Usa um arquivo temporario comum, herdado pelos processos filhos.
 * O pai le o registro apenas depois que o Simulador foi encerrado. */
static int analisar_registro(FILE *registro, const Cenario *cenario) {
    Resultado r = {0};
    char linha[1024];
    rewind(registro);
    while (fgets(linha, sizeof linha, registro)) {
        int id, pc, n, leituras, escritas, passos, repetidos, erros;
        long pid;
        char situacao[20];

        if (sscanf(linha, "[A%d] Finalizada (PC=%d, N=%d).", &id, &pc, &n) == 3) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES && pc == MAX_ITERACOES)
                r.aplicacao[id - 1]++;
            else r.erros++;
        } else if (sscanf(linha,
                   "[Kernel] A%d TERMINADO (PC=%d, N=%d, leituras=%d, escritas=%d).",
                   &id, &pc, &n, &leituras, &escritas) == 5) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES && pc == MAX_ITERACOES) {
                r.kernel[id - 1]++;
                r.leituras[id - 1] = leituras;
                r.escritas[id - 1] = escritas;
            } else r.erros++;
        } else if (sscanf(linha,
                   "[Validacao5000] A%d %19s passos=%d repetidos=%d erros=%d PC=%d",
                   &id, situacao, &passos, &repetidos, &erros, &pc) == 6) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES &&
                strcmp(situacao, "PASSOU") == 0 &&
                passos == MAX_ITERACOES && erros == 0 && pc == MAX_ITERACOES) {
                r.auditoria[id - 1]++;
                r.repetidos[id - 1] = repetidos;
            } else r.erros++;
        } else if (strstr(linha, "termino normal") &&
                   sscanf(linha, "[Simulador] Recolheu A%d (PID=%ld,",
                          &id, &pid) == 2) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES && pid > 0)
                r.recolhido[id - 1]++;
            else r.erros++;
        } else if (strstr(linha, "Termino confirmado pelo KernelSim.") &&
                   sscanf(linha, "[A%d]", &id) == 1) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES)
                r.confirmacao[id - 1]++;
            else r.erros++;
        } else if (sscanf(linha, "[Estado] A%d ", &id) == 1) {
            if (id >= 1 && id <= QUANTIDADE_APLICACOES)
                r.snapshots[id - 1]++;
        }

        if (strstr(linha, "[Kernel] Executando A")) r.escalonamentos++;
        if (strstr(linha, "[Kernel] A1 enviou PC=2 para A2.")) r.enviados++;
        if (strstr(linha, "[Kernel] A2 recebeu N=")) r.recebidos++;
        if (strstr(linha, "ESTADO DOS PROCESSOS (PAUSADO)")) r.pausas++;
        if (strstr(linha, "[Simulador] Execucao retomada.")) r.retomadas++;
        if (strstr(linha, "[Kernel] Todas as seis aplicacoes terminaram."))
            r.kernel_final++;
        if (strstr(linha, "[Simulador] As seis aplicacoes foram recolhidas."))
            r.simulador_final++;
    }

    int ok = r.erros == 0 && r.escalonamentos > QUANTIDADE_APLICACOES &&
             r.kernel_final == 1 && r.simulador_final == 1;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        int repeticoes_esperadas = cenario->com_syscalls && i < 2 ? 1 : 0;
        int valido = r.aplicacao[i] == 1 && r.kernel[i] == 1 &&
                     r.recolhido[i] == 1 && r.confirmacao[i] == 1 &&
                     r.auditoria[i] == 1 &&
                     r.repetidos[i] == repeticoes_esperadas;
        if (!valido) {
            fprintf(stderr, "[ERRO] A%d: app=%d kernel=%d recolhido=%d "
                            "confirmado=%d auditoria=%d repetidos=%d\n",
                    i + 1, r.aplicacao[i], r.kernel[i], r.recolhido[i],
                    r.confirmacao[i], r.auditoria[i], r.repetidos[i]);
            ok = 0;
        }
        if (cenario->com_pausa && r.snapshots[i] != 1) {
            fprintf(stderr, "[ERRO] A%d: snapshot ausente/duplicado.\n", i + 1);
            ok = 0;
        }
    }
    if (cenario->com_syscalls &&
        (r.enviados < 1 || r.recebidos < 1 ||
         r.escritas[0] < 1 || r.leituras[1] < 1)) {
        puts("[ERRO] SEND A1 / RECV A2 nao foram concluidos.");
        ok = 0;
    }
    if (cenario->com_pausa && (r.pausas != 1 || r.retomadas != 1)) {
        puts("[ERRO] Pausa/retomada nao foram confirmadas.");
        ok = 0;
    }
    if (!ok) {
        printf("[DIAGNOSTICO] %s: escalonamentos=%d, pausas=%d, "
               "retomadas=%d, erros=%d\n", cenario->nome,
               r.escalonamentos, r.pausas, r.retomadas, r.erros);
    } else {
        printf("[OK] %s: 6 x %d = %d iteracoes, %d escalonamentos.",
               cenario->nome, MAX_ITERACOES,
               QUANTIDADE_APLICACOES * MAX_ITERACOES, r.escalonamentos);
        if (cenario->com_syscalls) printf(" SEND/RECV concluidos.");
        if (cenario->com_pausa) printf(" Ctrl+Z pausou e retomou.");
        putchar('\n');
    }
    return ok;
}

static int executar_cenario(const Cenario *cenario) {
    FILE *registro = tmpfile();
    if (!registro) { perror("tmpfile"); return 0; }
    printf("[Validacao5000] Iniciando: %s...\n", cenario->nome);
    fflush(stdout);
    pid_t simulador = fork();
    if (simulador < 0) { perror("fork"); fclose(registro); return 0; }
    if (simulador == 0) {
        if (dup2(fileno(registro), STDOUT_FILENO) == -1 ||
            dup2(fileno(registro), STDERR_FILENO) == -1) _exit(127);
        fclose(registro);
        /* O tamanho da simulacao NAO e reduzido. Somente as esperas. */
        unsetenv("TESTE_MAX_ITERACOES");
        unsetenv("TESTE_SYSCALL");
        unsetenv("TESTE_SEM_SYSCALL");
        setenv("TESTE_INTERVALO_US", "500", 1);
        setenv("TESTE_INTERVALO_IRQ_US", "50000", 1);
        setenv("TESTE_VALIDAR_PC", "1", 1);
        setenv("TESTE_ENCERRAR_AO_FINAL", "1", 1);
        setenv(cenario->com_syscalls ? "TESTE_SYSCALL" : "TESTE_SEM_SYSCALL",
               "1", 1);
        execl("./Simulador", "Simulador", (char *)NULL);
        _exit(127);
    }

    int terminou = 0, estado = 0;
    for (int passo = 0; passo < 900; passo++) { /* Limite: 90 segundos. */
        pid_t r = waitpid(simulador, &estado, WNOHANG);
        if (r == simulador) { terminou = 1; break; }
        if (r < 0) break;
        /* Injetamos os mesmos SIGTSTP que Ctrl+Z enviaria ao Simulador. */
        if (cenario->com_pausa && passo == 10) kill(simulador, SIGTSTP);
        if (cenario->com_pausa && passo == 40) kill(simulador, SIGTSTP);
        usleep(100000);
    }
    if (!terminou) {
        fprintf(stderr, "[ERRO] Tempo excedido ou processo finalizado anormalmente: %s\n",
                cenario->nome);
        kill(simulador, SIGINT);
        for (int i = 0; i < 50; i++) {
            pid_t r = waitpid(simulador, &estado, WNOHANG);
            if (r == simulador) { terminou = 1; break; }
            if (r < 0) break;
            usleep(100000);
        }
        if (!terminou) {
            kill(simulador, SIGTERM);
            waitpid(simulador, &estado, 0);
        }
        fclose(registro);
        return 0;
    }
    if (!WIFEXITED(estado) || WEXITSTATUS(estado) != 0) {
        fprintf(stderr, "[ERRO] Simulador terminou com erro no cenario %s\n",
                cenario->nome);
        fclose(registro);
        return 0;
    }
    int ok = analisar_registro(registro, cenario);
    fclose(registro);
    return ok;
}

int main(void) {
    Cenario cenarios[] = {
        {"5000 iteracoes sem syscall", 0, 0},
        {"5000 iteracoes com SEND e RECV", 1, 0},
        {"5000 iteracoes com pausa e retomada", 0, 1}
    };
    int aprovados = 0;
    puts("=== VALIDACAO PROLONGADA DAS 5000 ITERACOES ===");
    for (size_t i = 0; i < sizeof cenarios / sizeof cenarios[0]; i++)
        if (executar_cenario(&cenarios[i])) aprovados++;
    printf("Resultado: %d de %zu cenarios passaram.\n",
           aprovados, sizeof cenarios / sizeof cenarios[0]);
    return aprovados == (int)(sizeof cenarios / sizeof cenarios[0]) ? 0 : 1;
}
