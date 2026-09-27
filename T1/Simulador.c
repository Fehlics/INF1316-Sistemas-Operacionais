#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t encerrar, alternar;

static void sair(int sinal) {
    (void)sinal;
    encerrar = 1;
}

static void tecla_z(int sinal) {
    (void)sinal;
    alternar = 1;
}

static int evento(int fd, int tipo) {
    Mensagem m = { .tipo = tipo };
    return write(fd, &m, sizeof m) == sizeof m;
}

static void mostrar(Fotografia f) {
    puts("===== PROCESSOS PAUSADOS =====");
    for (int i = 0; i < TOTAL; i++) {
        Processo p = f.processos[i];
        const char *estado[] = {"PRONTO", "EXECUTANDO", "BLOQUEADO", "TERMINADO"};
        printf("[Estado] A%d PC=%d N=%d %s CPU=%s ", i + 1,
               p.pc, p.n, estado[p.estado], f.atual == i ? "SIM" : "NAO");

        if (p.estado == BLOQUEADO) {
            printf("PIPE=%d %s ENDERECO=%s ", i / 2 + 1,
                   p.op == ENVIAR ? "SEND" : "RECV",
                   p.endereco == END_PC ? "PC" : "N");
        }

        printf("LEITURAS=%d ESCRITAS=%d TERMINADO=%s\n",
               p.leituras, p.escritas, p.estado == TERMINADO ? "SIM" : "NAO");
    }
}

static int mudar_pausa(int controle, int fd_estado, pid_t controlador,
                       pid_t filhos[], int *restantes, int pausado) {
    Fotografia f;
    int status;
    if (!pausado) {
        kill(controlador, SIGSTOP);
        if (waitpid(controlador, &status, WUNTRACED) != controlador ||
            !WIFSTOPPED(status)) {
            return 0;
        }

        if (!evento(controle, PAUSAR) ||
            read(fd_estado, &f, sizeof f) != sizeof f) {
            return 0;
        }

        /* O kernel confirma a ordem de parada; esperamos o estado real. */
        if (f.atual >= 0 && filhos[f.atual] > 0) {
            int i = f.atual;
            if (waitpid(filhos[i], &status, WUNTRACED) != filhos[i])
                return 0;

            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                filhos[i] = 0;
                (*restantes)--;
            } else if (!WIFSTOPPED(status)) {
                return 0;
            }
        }

        if (!evento(controle, MOSTRAR) ||
            read(fd_estado, &f, sizeof f) != sizeof f) {
            return 0;
        }
        mostrar(f);
    } else {
        if (!evento(controle, RETOMAR) ||
            read(fd_estado, &f, sizeof f) != sizeof f) {
            return 0;
        }
        kill(controlador, SIGCONT);
        puts("RETOMOU");
    }
    return 1;
}

int main(void) {
    int controle[2], estados[2], respostas[TOTAL][2];
    pid_t filhos[TOTAL] = {0}, kernel = 0, controlador = 0;
    char texto_controle[20], texto_resp[TOTAL][20], texto_pid[TOTAL][24];
    int erro = 0, restantes = TOTAL, pausado = 0;

    setbuf(stdout, NULL);
    signal(SIGINT, sair);
    signal(SIGTSTP, tecla_z);
    signal(SIGPIPE, SIG_IGN);
    if (pipe(controle) || pipe(estados))
        return 1;

    for (int i = 0; i < TOTAL; i++) {
        if (pipe(respostas[i]))
            return 1;
    }

    snprintf(texto_controle, sizeof texto_controle, "%d", controle[1]);
    for (int i = 0; i < TOTAL; i++) {
        snprintf(texto_resp[i], sizeof texto_resp[i], "%d", respostas[i][1]);
    }

    for (int i = 0; i < TOTAL; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            erro = 1;
            goto fim;
        }
        if (pid == 0) {
            signal(SIGTSTP, SIG_IGN);
            close(controle[0]);
            close(estados[0]);
            close(estados[1]);
            for (int j = 0; j < TOTAL; j++) {
                close(respostas[j][1]);
                if (j != i)
                    close(respostas[j][0]);
            }

            char id[8], fd[20];
            snprintf(id, sizeof id, "%d", i + 1);
            snprintf(fd, sizeof fd, "%d", respostas[i][0]);
            raise(SIGSTOP);
            execl("./Application", "Application", id, texto_controle,
                  fd, (char *)NULL);
            _exit(1);
        }

        filhos[i] = pid;
        int status;
        if (waitpid(pid, &status, WUNTRACED) != pid) {
            erro = 1;
            goto fim;
        }
        snprintf(texto_pid[i], sizeof texto_pid[i], "%ld", (long)pid);
    }

    kernel = fork();
    if (kernel < 0) {
        kernel = 0;
        erro = 1;
        goto fim;
    }
    if (kernel == 0) {
        signal(SIGTSTP, SIG_IGN);
        close(controle[1]);
        close(estados[0]);
        for (int i = 0; i < TOTAL; i++)
            close(respostas[i][0]);

        char fd_controle[20], fd_estado[20];
        snprintf(fd_controle, sizeof fd_controle, "%d", controle[0]);
        snprintf(fd_estado, sizeof fd_estado, "%d", estados[1]);
        execl("./KernelSim", "KernelSim", fd_controle,
              texto_resp[0], texto_resp[1], texto_resp[2],
              texto_resp[3], texto_resp[4], texto_resp[5],
              texto_pid[0], texto_pid[1], texto_pid[2],
              texto_pid[3], texto_pid[4], texto_pid[5], fd_estado, (char *)NULL);
        _exit(1);
    }

    controlador = fork();
    if (controlador < 0) {
        controlador = 0;
        erro = 1;
        goto fim;
    }
    if (controlador == 0) {
        signal(SIGTSTP, SIG_IGN);
        close(controle[0]);
        close(estados[0]);
        close(estados[1]);
        for (int i = 0; i < TOTAL; i++) {
            close(respostas[i][0]);
            close(respostas[i][1]);
        }
        execl("./InterController", "InterController", texto_controle,
              (char *)NULL);
        _exit(1);
    }

    close(controle[0]);
    close(estados[1]);
    for (int i = 0; i < TOTAL; i++) {
        close(respostas[i][0]);
        close(respostas[i][1]);
    }

    puts("PRONTO - Ctrl+Z pausa/retoma, Ctrl+C encerra");
    while (!encerrar) {
        if (alternar) {
            alternar = 0;
            if (!mudar_pausa(controle[1], estados[0], controlador,
                             filhos, &restantes, pausado)) {
                erro = 1;
                break;
            }
            pausado = !pausado;
        }

        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid == kernel || pid == controlador) {
            erro = 1;
            break;
        }
        if (pid > 0) {
            for (int i = 0; i < TOTAL; i++) {
                if (filhos[i] != pid)
                    continue;
                filhos[i] = 0;
                restantes--;
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    printf("RECOLHEU A%d\n", i + 1);
                else
                    erro = 1;
                break;
            }
        }

        if (!restantes && getenv("TESTE_AUTO"))
            break;
        sleep(1);
    }

fim:
    if (controlador > 0) {
        kill(controlador, SIGTERM);
        kill(controlador, SIGCONT);
    }
    if (kernel > 0)
        kill(kernel, SIGTERM);
    for (int i = 0; i < TOTAL; i++) {
        if (filhos[i] > 0) {
            kill(filhos[i], SIGCONT);
            kill(filhos[i], SIGTERM);
        }
    }

    if (controlador > 0)
        waitpid(controlador, NULL, 0);
    if (kernel > 0)
        waitpid(kernel, NULL, 0);
    for (int i = 0; i < TOTAL; i++) {
        if (filhos[i] > 0)
            waitpid(filhos[i], NULL, 0);
    }

    if (kernel > 0) {
        close(controle[1]);
        close(estados[0]);
    }
    puts("SIMULADOR ENCERRADO");
    return erro ? 1 : encerrar ? 130 : 0;
}
