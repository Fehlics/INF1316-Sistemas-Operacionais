#include "trabalho.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* O tratador so altera uma variavel, como nos exemplos de sinais. */
static volatile sig_atomic_t encerrar = 0;
static void ao_interromper(int sinal) {
    (void)sinal;
    encerrar = 1;
}

static void finalizar(pid_t aplicativos[], pid_t kernel, pid_t controlador) {
    if (controlador > 0) kill(controlador, SIGTERM);
    if (kernel > 0) kill(kernel, SIGTERM);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (aplicativos[i] > 0) {
            kill(aplicativos[i], SIGCONT);
            kill(aplicativos[i], SIGTERM);
        }
    }
    if (controlador > 0) waitpid(controlador, NULL, 0);
    if (kernel > 0) waitpid(kernel, NULL, 0);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (aplicativos[i] > 0) waitpid(aplicativos[i], NULL, 0);
}

int main(void) {
    int controle[2]; /* InterController e Application escrevem; KernelSim le. */
    int respostas[QUANTIDADE_APLICACOES][2];
    pid_t aplicativos[QUANTIDADE_APLICACOES] = {0};
    pid_t kernel = -1, controlador = -1;
    char pids[QUANTIDADE_APLICACOES][32];
    char texto_respostas[QUANTIDADE_APLICACOES][32];
    char texto_controle[32];
    setbuf(stdout, NULL);
    signal(SIGINT, ao_interromper);

    if (pipe(controle) == -1) { perror("pipe controle"); return 1; }
    snprintf(texto_controle, sizeof texto_controle, "%d", controle[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (pipe(respostas[i]) == -1) {
            perror("pipe resposta");
            for (int j = 0; j < i; j++) {
                close(respostas[j][0]); close(respostas[j][1]);
            }
            close(controle[0]); close(controle[1]);
            return 1;
        }
        snprintf(texto_respostas[i], sizeof texto_respostas[i], "%d",
                 respostas[i][1]);
    }

    /* As seis aplicacoes comecam suspensas. */
    for (int i = 0; i < QUANTIDADE_APLICACOES && !encerrar; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork Application");
            encerrar = 1;
            break;
        }
        if (pid == 0) {
            char id[16], fd_resposta[32];
            signal(SIGINT, SIG_DFL);
            close(controle[0]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
                close(respostas[j][1]);
                if (j != i) close(respostas[j][0]);
            }
            snprintf(id, sizeof id, "%d", i + 1);
            snprintf(fd_resposta, sizeof fd_resposta, "%d", respostas[i][0]);
            raise(SIGSTOP);
            execl("./Application", "Application", id,
                  texto_controle, fd_resposta, (char *)NULL);
            perror("exec Application");
            _exit(1);
        }
        aplicativos[i] = pid;
        int status;
        while (waitpid(pid, &status, WUNTRACED) == -1 && errno == EINTR) {
            if (encerrar) break;
        }
        snprintf(pids[i], sizeof pids[i], "%ld", (long)pid);
        printf("[Simulador] Criou A%d PID=%ld\n", i + 1, (long)pid);
    }

    if (!encerrar) {
        kernel = fork();
        if (kernel < 0) {
            perror("fork KernelSim"); encerrar = 1;
        } else if (kernel == 0) {
            char fd_leitura[32];
            signal(SIGINT, SIG_DFL);
            close(controle[1]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++)
                close(respostas[j][0]);
            snprintf(fd_leitura, sizeof fd_leitura, "%d", controle[0]);
            execl("./KernelSim", "KernelSim", fd_leitura,
                  texto_respostas[0], texto_respostas[1], texto_respostas[2],
                  texto_respostas[3], texto_respostas[4], texto_respostas[5],
                  pids[0], pids[1], pids[2], pids[3], pids[4], pids[5],
                  (char *)NULL);
            perror("exec KernelSim");
            _exit(1);
        } else {
            printf("[Simulador] Criou KernelSim PID=%ld\n", (long)kernel);
        }
    }

    if (!encerrar) {
        controlador = fork();
        if (controlador < 0) {
            perror("fork InterController"); encerrar = 1;
        } else if (controlador == 0) {
            signal(SIGINT, SIG_DFL);
            close(controle[0]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
                close(respostas[j][0]); close(respostas[j][1]);
            }
            execl("./InterController", "InterController", texto_controle,
                  (char *)NULL);
            perror("exec InterController");
            _exit(1);
        } else {
            printf("[Simulador] Criou InterController PID=%ld\n",
                   (long)controlador);
        }
    }

    close(controle[0]); close(controle[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        close(respostas[i][0]); close(respostas[i][1]);
    }

    if (!encerrar) {
        puts("[Simulador] Processos iniciados. Ctrl+C para encerrar.");
        /* Ainda falta a pausa coordenada com Ctrl+Z. */
        while (!encerrar) {
            int estado;
            pid_t terminou = waitpid(kernel, &estado, WNOHANG);
            if (terminou == kernel || terminou == -1) { kernel = -1; break; }
            sleep(1);
        }
    }
    finalizar(aplicativos, kernel, controlador);
    puts("[Simulador] Encerrado.");
    return encerrar ? 130 : 0;
}
