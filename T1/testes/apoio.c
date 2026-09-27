#include "apoio.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* O tratador escreve apenas um byte na pipe de avisos. */
static int fd_aviso;
static char numero_aplicacao;
static void avisar_retomada(int sinal) {
    (void)sinal;
    write(fd_aviso, &numero_aplicacao, 1);
}

static void fechar(int *fd) {
    if (*fd >= 0) close(*fd);
    *fd = -1;
}

void encerrar_ambiente(AmbienteTeste *a) {
    /* Primeiro desligamos o kernel, depois recolhemos os auxiliares. */
    if (a->kernel > 0) {
        kill(a->kernel, SIGTERM);
        waitpid(a->kernel, NULL, 0);
        a->kernel = -1;
    }
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (a->auxiliares[i] > 0) {
            kill(a->auxiliares[i], SIGCONT);
            kill(a->auxiliares[i], SIGTERM);
            waitpid(a->auxiliares[i], NULL, 0);
            a->auxiliares[i] = -1;
        }
        fechar(&a->resposta[i]);
    }
    fechar(&a->controle);
    fechar(&a->avisos);
    fechar(&a->estados);
}

int esperar_ativo(AmbienteTeste *a, int id) {
    char recebido;
    ssize_t n;
    do { n = read(a->avisos, &recebido, 1); }
    while (n < 0 && errno == EINTR);
    if (n == 1 && recebido == (char)id) return 1;
    fprintf(stderr, "[Teste] Esperava retomada A%d; recebi A%d.\n",
            id, n == 1 ? (int)recebido : -1);
    return 0;
}

int verificar_parada(pid_t pid) {
    /* Depois do SIGSTOP, waitpid notifica o pai sobre o novo estado. */
    for (int i = 0; i < 50; i++) {
        int status;
        pid_t retorno = waitpid(pid, &status, WNOHANG | WUNTRACED);
        if (retorno == pid) return WIFSTOPPED(status);
        if (retorno < 0) return 0;
        usleep(10000);
    }
    return 0;
}

int iniciar_ambiente(AmbienteTeste *a) {
    int canal[2] = {-1, -1}, avisos[2] = {-1, -1};
    int estados[2] = {-1, -1};
    int respostas[QUANTIDADE_APLICACOES][2];
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        a->auxiliares[i] = -1;
        a->resposta[i] = -1;
        respostas[i][0] = respostas[i][1] = -1;
    }
    a->controle = a->avisos = a->estados = -1;
    a->kernel = -1;
    if (pipe(canal) == -1 || pipe(avisos) == -1 || pipe(estados) == -1)
        goto falha;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (pipe(respostas[i]) == -1) goto falha;

    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pid_t pid = fork();
        if (pid < 0) goto falha;
        if (pid == 0) {
            fd_aviso = avisos[1];
            numero_aplicacao = (char)(i + 1);
            close(avisos[0]);
            close(estados[0]); close(estados[1]);
            close(canal[0]); close(canal[1]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
                close(respostas[j][0]); close(respostas[j][1]);
            }
            signal(SIGCONT, avisar_retomada);
            raise(SIGSTOP);
            for (;;) pause();
        }
        a->auxiliares[i] = pid;
        int estado;
        if (waitpid(pid, &estado, WUNTRACED) != pid ||
            !WIFSTOPPED(estado)) goto falha;
    }

    a->kernel = fork();
    if (a->kernel < 0) { a->kernel = -1; goto falha; }
    if (a->kernel == 0) {
        close(canal[1]); close(avisos[0]); close(avisos[1]);
        close(estados[0]);
        char textos[QUANTIDADE_APLICACOES * 2 + 2][32];
        char *argumentos[QUANTIDADE_APLICACOES * 2 + 4];
        argumentos[0] = "./KernelSim";
        snprintf(textos[0], sizeof textos[0], "%d", canal[0]);
        argumentos[1] = textos[0];
        for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
            close(respostas[i][0]);
            snprintf(textos[i + 1], sizeof textos[i + 1], "%d", respostas[i][1]);
            argumentos[i + 2] = textos[i + 1];
            snprintf(textos[i + 1 + QUANTIDADE_APLICACOES],
                     sizeof textos[i + 1 + QUANTIDADE_APLICACOES], "%ld",
                     (long)a->auxiliares[i]);
            argumentos[i + 2 + QUANTIDADE_APLICACOES] =
                textos[i + 1 + QUANTIDADE_APLICACOES];
        }
        snprintf(textos[QUANTIDADE_APLICACOES * 2 + 1],
                 sizeof textos[QUANTIDADE_APLICACOES * 2 + 1], "%d", estados[1]);
        argumentos[QUANTIDADE_APLICACOES * 2 + 2] =
            textos[QUANTIDADE_APLICACOES * 2 + 1];
        argumentos[QUANTIDADE_APLICACOES * 2 + 3] = NULL;
        /* Testamos os sinais usando avisos; nao precisamos analisar logs. */
        FILE *saida = fopen("/dev/null", "w");
        if (saida) {
            dup2(fileno(saida), STDOUT_FILENO);
            fclose(saida);
        }
        execv("./KernelSim", argumentos);
        _exit(127);
    }

    fechar(&canal[0]);
    fechar(&avisos[1]);
    fechar(&estados[1]);
    a->estados = estados[0]; estados[0] = -1;
    a->controle = canal[1]; canal[1] = -1;
    a->avisos = avisos[0]; avisos[0] = -1;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        fechar(&respostas[i][1]);
        a->resposta[i] = respostas[i][0];
        respostas[i][0] = -1;
    }
    if (!esperar_ativo(a, 1)) goto falha;
    return 1;

falha:
    fechar(&canal[0]); fechar(&canal[1]);
    fechar(&avisos[0]); fechar(&avisos[1]);
    fechar(&estados[0]); fechar(&estados[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        fechar(&respostas[i][0]); fechar(&respostas[i][1]);
    }
    encerrar_ambiente(a);
    return 0;
}

int enviar_pedido_teste(AmbienteTeste *a, int id, Operacao op, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVENTO_SYSCALL;
    m.pedido.id_aplicacao = id;
    m.pedido.operacao = op;
    m.pedido.pc = pc;
    m.pedido.n = n;
    return write(a->controle, &m, sizeof m) == (ssize_t)sizeof m;
}

int enviar_termino_teste(AmbienteTeste *a, int id, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVENTO_TERMINO;
    m.pedido.id_aplicacao = id;
    m.pedido.operacao = NENHUMA_OPERACAO;
    m.pedido.pc = pc;
    m.pedido.n = n;
    return write(a->controle, &m, sizeof m) == (ssize_t)sizeof m;
}

int enviar_irq_teste(AmbienteTeste *a, TipoIRQ irq) {
    MensagemControle m = {0};
    m.tipo = EVENTO_INTERRUPCAO;
    m.irq = irq;
    return write(a->controle, &m, sizeof m) == (ssize_t)sizeof m;
}

int ler_resposta_teste(AmbienteTeste *a, int id, Operacao op, int n) {
    RespostaSyscall r;
    ssize_t lidos;
    do { lidos = read(a->resposta[id - 1], &r, sizeof r); }
    while (lidos < 0 && errno == EINTR);
    return lidos == (ssize_t)sizeof r && r.id_aplicacao == id &&
           r.operacao == op && r.n == n;
}
