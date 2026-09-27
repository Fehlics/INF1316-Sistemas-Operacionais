#include "trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t encerrar = 0;
volatile sig_atomic_t pausar = 0;

int controle[2], estados[2], respostas[TOTAL][2];
pid_t filhos[TOTAL] = {0};
pid_t kernel = 0;
pid_t controlador = 0;
char texto_controle[20];
char texto_resp[TOTAL][20];
char texto_pid[TOTAL][24];
int restantes = TOTAL;

void ctrl_c(int sinal) {
    (void)sinal;
    encerrar = 1;
}

void ctrl_z(int sinal) {
    (void)sinal;
    pausar = 1;
}

int enviar_evento(int tipo) {
    Mensagem m = {0};
    m.tipo = tipo;
    return write(controle[1], &m, sizeof m) == sizeof m;
}

void mostrar(Fotografia f) {
    char *nomes[] = {"PRONTO", "EXECUTANDO", "BLOQUEADO", "TERMINADO"};
    puts("===== PROCESSOS PAUSADOS =====");
    for (int i = 0; i < TOTAL; i++) {
        Processo p = f.processos[i];
        char *cpu = "NAO";
        if (f.atual == i)
            cpu = "SIM";
        printf("A%d PC=%d N=%d ESTADO=%s CPU=%s ", i + 1, p.pc, p.n, nomes[p.estado], cpu);

        if (p.estado == BLOQUEADO) {
            char *operacao = "RECV";
            char *endereco = "N";
            if (p.op == ENVIAR)
                operacao = "SEND";
            if (p.endereco == END_PC)
                endereco = "PC";
            printf("PIPE=%d OPERACAO=%s ENDERECO=%s ", i / 2 + 1, operacao, endereco);
        }
        char *terminado = "NAO";
        if (p.estado == TERMINADO)
            terminado = "SIM";
        printf("LEITURAS=%d ESCRITAS=%d TERMINADO=%s\n", p.leituras, p.escritas, terminado);
    }
}

int mudar_pausa(int parado) {
    Fotografia f;
    int status;

    if (!parado) {
        kill(controlador, SIGSTOP);
        if (waitpid(controlador, &status, WUNTRACED) != controlador)
            return 0;

        if (!enviar_evento(PAUSAR))
            return 0;
        if (read(estados[0], &f, sizeof f) != sizeof f)
            return 0;

        if (f.atual >= 0 && filhos[f.atual] > 0) {
            int i = f.atual;
            if (waitpid(filhos[i], &status, WUNTRACED) != filhos[i])
                return 0;
            if (WIFEXITED(status)) {
                filhos[i] = 0;
                restantes--;
            }
        }

        if (!enviar_evento(MOSTRAR))
            return 0;
        if (read(estados[0], &f, sizeof f) != sizeof f)
            return 0;
        mostrar(f);
    } else {
        if (!enviar_evento(RETOMAR))
            return 0;
        if (read(estados[0], &f, sizeof f) != sizeof f)
            return 0;
        kill(controlador, SIGCONT);
        puts("SIMULACAO RETOMADA");
    }
    return 1;
}

int criar_canais(void) {
    if (pipe(controle) != 0 || pipe(estados) != 0)
        return 0;

    for (int i = 0; i < TOTAL; i++) {
        if (pipe(respostas[i]) != 0)
            return 0;
    }
    snprintf(texto_controle, sizeof texto_controle, "%d", controle[1]);
    for (int i = 0; i < TOTAL; i++)
        snprintf(texto_resp[i], sizeof texto_resp[i], "%d", respostas[i][1]);
    return 1;
}

int criar_aplicacoes(void) {
    for (int i = 0; i < TOTAL; i++) {
        pid_t pid = fork();
        if (pid < 0)
            return 0;
        if (pid == 0) {
            signal(SIGTSTP, SIG_IGN);
            close(controle[0]);
            close(estados[0]);
            close(estados[1]);
            for (int j = 0; j < TOTAL; j++) {
                close(respostas[j][1]);
                if (i != j)
                    close(respostas[j][0]);
            }

            char id[8], fd[20];
            snprintf(id, sizeof id, "%d", i + 1);
            snprintf(fd, sizeof fd, "%d", respostas[i][0]);
            raise(SIGSTOP);
            execl("./Application", "Application", id, texto_controle, fd, (char *)NULL);
            exit(1);
        }

        filhos[i] = pid;
        int status;
        if (waitpid(pid, &status, WUNTRACED) != pid)
            return 0;
        snprintf(texto_pid[i], sizeof texto_pid[i], "%ld", (long)pid);
    }
    return 1;
}

int criar_kernel(void) {
    kernel = fork();
    if (kernel < 0) {
        kernel = 0;
        return 0;
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
        execl("./KernelSim", "KernelSim", fd_controle, texto_resp[0], texto_resp[1], texto_resp[2], texto_resp[3], texto_resp[4], texto_resp[5], texto_pid[0], texto_pid[1], texto_pid[2], texto_pid[3], texto_pid[4], texto_pid[5], fd_estado, (char *)NULL);
        exit(1);
    }
    return 1;
}

int criar_controlador(void) {
    controlador = fork();
    if (controlador < 0) {
        controlador = 0;
        return 0;
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
        execl("./InterController", "InterController", texto_controle, (char *)NULL);
        exit(1);
    }
    return 1;
}

void finalizar(void) {
    if (controlador > 0) {
        kill(controlador, SIGTERM);
        kill(controlador, SIGCONT);
    }
    if (kernel > 0)
        kill(kernel, SIGTERM);

    for (int i = 0; i < TOTAL; i++) {
        if (filhos[i] > 0) {
            kill(filhos[i], SIGTERM);
            kill(filhos[i], SIGCONT);
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
}

int main(void) {
    int erro = 0;
    int parado = 0;
    setbuf(stdout, NULL);
    signal(SIGINT, ctrl_c);
    signal(SIGTSTP, ctrl_z);
    signal(SIGPIPE, SIG_IGN);

    if (!criar_canais())
        return 1;
    if (!criar_aplicacoes() || !criar_kernel() || !criar_controlador()) {
        erro = 1;
    } else {
        close(controle[0]);
        close(estados[1]);
        for (int i = 0; i < TOTAL; i++) {
            close(respostas[i][0]);
            close(respostas[i][1]);
        }
        puts("PRONTO - Ctrl+Z pausa/retoma, Ctrl+C encerra");

        while (!encerrar) {
            if (pausar) {
                pausar = 0;
                if (!mudar_pausa(parado)) {
                    erro = 1;
                    break;
                }
                parado = !parado;
            }

            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG);
            if (pid == kernel || pid == controlador) {
                erro = 1;
                break;
            }
            if (pid > 0) {
                for (int i = 0; i < TOTAL; i++) {
                    if (filhos[i] == pid) {
                        filhos[i] = 0;
                        restantes--;
                        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                            printf("RECOLHEU A%d\n", i + 1);
                        else
                            erro = 1;
                    }
                }
            }
            if (restantes == 0 && getenv("TESTE_AUTO") != NULL)
                break;
            sleep(1);
        }
        close(controle[1]);
        close(estados[0]);
    }

    finalizar();
    puts("SIMULADOR ENCERRADO");
    if (erro)
        return 1;
    if (encerrar)
        return 130;
    return 0;
}