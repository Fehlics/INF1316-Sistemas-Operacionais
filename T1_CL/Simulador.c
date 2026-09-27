#include "trabalho.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t encerrar = 0, alternar = 0;
static void ao_sigint(int s) { (void)s; encerrar = 1; }
static void ao_sigtstp(int s) { (void)s; alternar = 1; }

static int enviar_evento(int fd, TipoEvento tipo) {
    MensagemControle m = {0};
    m.tipo = tipo;
    return write(fd, &m, sizeof m) == (ssize_t)sizeof m;
}

static int receber_estado(int fd, EstadoSimulador *e) {
    size_t total = 0;
    while (total < sizeof *e) {
        ssize_t n = read(fd, (char *)e + total, sizeof *e - total);
        if (n <= 0) return 0;
        total += (size_t)n;
    }
    return 1;
}

static void mostrar_estados(const EstadoSimulador *e) {
    printf("\n===== ESTADO DOS PROCESSOS (PAUSADO) =====\n");
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        const Processo *p = &e->processos[i];
        const char *nome_estado = p->estado == TERMINADO ? "TERMINADO" :
                                   p->estado == BLOQUEADO ? "BLOQUEADO" : "PRONTO";
        printf("A%d PC=%d N=%d ESTADO=%s", p->id, p->pc, p->n, nome_estado);
        if (p->estado == BLOQUEADO)
            printf(" DISPOSITIVO=pipe%d OPERACAO=%s", i / 2 + 1,
                   p->operacao_pendente == ENVIAR ? "SEND" : "RECV");
        printf(" EXECUTANDO=%s LEITURAS=%d ESCRITAS=%d\n",
               e->executando == p->id ? "sim" : "nao", p->leituras, p->escritas);
    }
    puts("Ctrl+Z novamente para retomar.\n");
}

int main(void) {
    int controle[2], canal_estado[2], respostas[QUANTIDADE_APLICACOES][2];
    pid_t aplicativos[QUANTIDADE_APLICACOES], kernel, controlador;
    char pids[QUANTIDADE_APLICACOES][32], texto_resp[QUANTIDADE_APLICACOES][32];
    char texto_controle[32], texto_estado[32];
    setbuf(stdout, NULL);
    signal(SIGINT, ao_sigint);
    signal(SIGTSTP, ao_sigtstp);
    signal(SIGPIPE, SIG_IGN);

    pipe(controle);
    pipe(canal_estado);
    snprintf(texto_controle, sizeof texto_controle, "%d", controle[1]);
    snprintf(texto_estado, sizeof texto_estado, "%d", canal_estado[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pipe(respostas[i]);
        snprintf(texto_resp[i], sizeof texto_resp[i], "%d", respostas[i][1]);
    }

    /* Cria as seis aplicacoes, ja suspensas. */
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char id[16], fd_resp[32];
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_IGN);
            close(canal_estado[0]); close(canal_estado[1]);
            close(controle[0]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
                close(respostas[j][1]);
                if (j != i) close(respostas[j][0]);
            }
            snprintf(id, sizeof id, "%d", i + 1);
            snprintf(fd_resp, sizeof fd_resp, "%d", respostas[i][0]);
            raise(SIGSTOP);
            execl("./Application", "Application", id, texto_controle, fd_resp, NULL);
            _exit(1);
        }
        aplicativos[i] = pid;
        int status;
        waitpid(pid, &status, WUNTRACED);
        snprintf(pids[i], sizeof pids[i], "%ld", (long)pid);
        printf("[Simulador] Criou A%d PID=%ld\n", i + 1, (long)pid);
    }

    kernel = fork();
    if (kernel == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_IGN);
        close(canal_estado[0]);
        close(controle[1]);
        for (int j = 0; j < QUANTIDADE_APLICACOES; j++) close(respostas[j][0]);
        char fd_leitura[32];
        snprintf(fd_leitura, sizeof fd_leitura, "%d", controle[0]);
        execl("./KernelSim", "KernelSim", fd_leitura,
              texto_resp[0], texto_resp[1], texto_resp[2],
              texto_resp[3], texto_resp[4], texto_resp[5],
              pids[0], pids[1], pids[2], pids[3], pids[4], pids[5],
              texto_estado, NULL);
        _exit(1);
    }
    printf("[Simulador] Criou KernelSim PID=%ld\n", (long)kernel);

    controlador = fork();
    if (controlador == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_IGN);
        close(canal_estado[0]); close(canal_estado[1]);
        close(controle[0]);
        for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
            close(respostas[j][0]); close(respostas[j][1]);
        }
        execl("./InterController", "InterController", texto_controle, NULL);
        _exit(1);
    }
    printf("[Simulador] Criou InterController PID=%ld\n", (long)controlador);

    close(controle[0]);
    close(canal_estado[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        close(respostas[i][0]); close(respostas[i][1]);
    }

    puts("[Simulador] Processos iniciados. Ctrl+Z pausa/retoma; Ctrl+C encerra.");
    int pausado = 0, restantes = QUANTIDADE_APLICACOES;
    while (!encerrar) {
        if (alternar) {
            alternar = 0;
            if (!pausado) {
                EstadoSimulador estado;
                enviar_evento(controle[1], EVT_PAUSAR);
                if (receber_estado(canal_estado[0], &estado)) mostrar_estados(&estado);
            } else {
                enviar_evento(controle[1], EVT_RETOMAR);
                puts("[Simulador] Execucao retomada.");
            }
            pausado = !pausado;
        }
        int status;
        pid_t terminou = waitpid(-1, &status, WNOHANG);
        if (terminou > 0) {
            for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
                if (terminou == aplicativos[i] && aplicativos[i] > 0) {
                    aplicativos[i] = 0;
                    restantes--;
                    printf("[Simulador] Recolheu A%d.\n", i + 1);
                    if (restantes == 0)
                        puts("[Simulador] As seis aplicacoes foram recolhidas.");
                }
            }
        }
        sleep(1);
    }

    kill(kernel, SIGTERM);
    kill(controlador, SIGTERM);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (aplicativos[i] > 0) { kill(aplicativos[i], SIGCONT); kill(aplicativos[i], SIGTERM); }
    waitpid(kernel, NULL, 0);
    waitpid(controlador, NULL, 0);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (aplicativos[i] > 0) waitpid(aplicativos[i], NULL, 0);
    puts("[Simulador] Encerrado.");
    return 0;
}
