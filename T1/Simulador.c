#include "trabalho.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* A flag é alterada pelo sinal e consultada fora do tratador. */
static volatile sig_atomic_t encerrar = 0;

static void ao_interromper(int sinal) {
    (void)sinal;
    encerrar = 1;
}

/* Encerra os processos que o simulador conseguiu criar. */
static void finalizar(pid_t aplicativos[], pid_t kernel, pid_t controlador) {
    if (controlador > 0) kill(controlador, SIGTERM);
    if (kernel > 0) kill(kernel, SIGTERM);

    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (aplicativos[i] > 0) {
            /* Um processo parado por SIGSTOP precisa ser retomado. */
            kill(aplicativos[i], SIGCONT);
            kill(aplicativos[i], SIGTERM);
        }
    }

    if (controlador > 0) waitpid(controlador, NULL, 0);
    if (kernel > 0) waitpid(kernel, NULL, 0);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (aplicativos[i] > 0) waitpid(aplicativos[i], NULL, 0);
    }
}

int main(void) {
    int canal_irq[2];
    int canal_syscall[2]; /* Application -> KernelSim. */
    pid_t aplicativos[QUANTIDADE_APLICACOES] = {0};
    pid_t kernel = -1;
    pid_t controlador = -1;
    char texto_pids[QUANTIDADE_APLICACOES][32];
    char texto_fd[32];
    char texto_fd_syscall[32];

    struct sigaction acao = {0};
    acao.sa_handler = ao_interromper;
    sigemptyset(&acao.sa_mask);
    sigaction(SIGINT, &acao, NULL); /* Ctrl+C encerra a demonstração. */

    setbuf(stdout, NULL); /* Exibe mensagens imediatamente. */

    /* Pipe real: InterController escreve IRQs; KernelSim lê IRQs. */
    if (pipe(canal_irq) == -1) {
        perror("pipe");
        return 1;
    }

    /*
     * Segundo pipe real: as seis aplicacoes escrevem seus pedidos na
     * MESMA ponta; o KernelSim e o unico leitor.
     */
    if (pipe(canal_syscall) == -1) {
        perror("pipe syscall");
        close(canal_irq[0]);
        close(canal_irq[1]);
        return 1;
    }
    snprintf(texto_fd_syscall, sizeof texto_fd_syscall, "%d", canal_syscall[1]);

    /* Cria os seis processos de aplicação, inicialmente PARADOS. */
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (encerrar) break;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork Application");
            encerrar = 1;
            break;
        }
        if (pid == 0) {
            char id[16];
            signal(SIGINT, SIG_DFL); /* Não herda o tratador do pai. */
            close(canal_irq[0]);
            close(canal_irq[1]);
            close(canal_syscall[0]); /* A aplicacao so escreve pedidos. */
            snprintf(id, sizeof id, "%d", i + 1);
            raise(SIGSTOP); /* O KernelSim dará o primeiro SIGCONT. */
            execl("./Application", "Application", id,
                  texto_fd_syscall, (char *)NULL);
            perror("exec Application");
            _exit(1);
        }
        aplicativos[i] = pid;

        /* Garante que cada aplicação parou antes de iniciar o kernel. */
        int estado;
        while (waitpid(pid, &estado, WUNTRACED) == -1 && errno == EINTR) {
            if (encerrar) break;
        }
        snprintf(texto_pids[i], sizeof texto_pids[i], "%ld", (long)pid);
    }

    if (!encerrar) {
        kernel = fork();
        if (kernel < 0) {
            perror("fork KernelSim");
            encerrar = 1;
        } else if (kernel == 0) {
            signal(SIGINT, SIG_DFL);
            close(canal_irq[1]);
            close(canal_syscall[1]); /* O kernel so le pedidos. */
            snprintf(texto_fd, sizeof texto_fd, "%d", canal_irq[0]);
            char fd_syscall[32];
            snprintf(fd_syscall, sizeof fd_syscall, "%d", canal_syscall[0]);
            execl("./KernelSim", "KernelSim", texto_fd, fd_syscall,
                  texto_pids[0], texto_pids[1], texto_pids[2],
                  texto_pids[3], texto_pids[4], texto_pids[5],
                  (char *)NULL);
            perror("exec KernelSim");
            _exit(1);
        }
    }

    if (!encerrar) {
        controlador = fork();
        if (controlador < 0) {
            perror("fork InterController");
            encerrar = 1;
        } else if (controlador == 0) {
            signal(SIGINT, SIG_DFL);
            close(canal_irq[0]);
            close(canal_syscall[0]); /* O controlador nao usa syscalls. */
            close(canal_syscall[1]);
            snprintf(texto_fd, sizeof texto_fd, "%d", canal_irq[1]);
            execl("./InterController", "InterController", texto_fd,
                  (char *)NULL);
            perror("exec InterController");
            _exit(1);
        }
    }

    close(canal_irq[0]);
    close(canal_irq[1]);
    close(canal_syscall[0]);
    close(canal_syscall[1]);

    if (!encerrar) {
        printf("[Simulador] Processos iniciados. Ctrl+C para encerrar.\n");
        /* TODO: receber estados do KernelSim para a pausa com Ctrl+Z. */
        while (waitpid(kernel, NULL, 0) == -1 && errno == EINTR) {
            if (encerrar) break;
        }
        /* O kernel já foi recolhido caso waitpid tenha terminado. */
        if (!encerrar) kernel = -1;
    }

    finalizar(aplicativos, kernel, controlador);
    puts("[Simulador] Encerrado.");
    return encerrar ? 130 : 0;
}
