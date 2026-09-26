#include "testes.h"
#include "../trabalho.h"

#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Teste deterministico: inicia KernelSim com 6 filhos controlados,
 * injeta syscalls e interrupcoes, sem depender do InterController aleatorio.
 */
static int disponivel(int fd, int milissegundos) {
    struct pollfd item = { .fd = fd, .events = POLLIN };
    return poll(&item, 1, milissegundos) > 0 && (item.revents & POLLIN);
}

static int pedido(int fd, int id, Operacao operacao) {
    PedidoSyscall p = { .id_aplicacao = id, .operacao = operacao,
                        .pc = id * 10, .n = id * 100 };
    return write(fd, &p, sizeof p) == (ssize_t)sizeof p;
}

static int irq(int fd, TipoIRQ tipo) {
    MensagemIRQ mensagem = { .tipo = tipo };
    return write(fd, &mensagem, sizeof mensagem) == (ssize_t)sizeof mensagem;
}

static int resposta(int fd, int id, Operacao operacao, int n) {
    RespostaSyscall r;
    if (!disponivel(fd, 1000)) return 0;
    if (read(fd, &r, sizeof r) != (ssize_t)sizeof r) return 0;
    return r.id_aplicacao == id && r.operacao == operacao && r.n == n;
}

int testar_filas(void) {
    int canal_irq[2], canal_pedidos[2], respostas[6][2];
    pid_t filhos[6] = {0}, kernel = -1;
    int criados = 0, resultado = 0;
    if (pipe(canal_irq) || pipe(canal_pedidos)) return 0;
    for (int i = 0; i < 6; i++) {
        if (pipe(respostas[i])) return 0;
    }
    for (int i = 0; i < 6; i++) {
        pid_t pid = fork();
        if (pid < 0) goto limpar;
        if (pid == 0) {
            raise(SIGSTOP);
            for (;;) pause();
        }
        filhos[i] = pid;
        criados++;
        int status;
        if (waitpid(pid, &status, WUNTRACED) != pid || !WIFSTOPPED(status))
            goto limpar;
    }
    kernel = fork();
    if (kernel < 0) goto limpar;
    if (kernel == 0) {
        char texto[14][32], *args[16];
        args[0] = "./KernelSim";
        snprintf(texto[0], sizeof texto[0], "%d", canal_irq[0]);
        snprintf(texto[1], sizeof texto[1], "%d", canal_pedidos[0]);
        args[1] = texto[0];
        args[2] = texto[1];
        for (int i = 0; i < 6; i++) {
            close(respostas[i][0]);
            snprintf(texto[2 + i], sizeof texto[2 + i], "%d", respostas[i][1]);
            snprintf(texto[8 + i], sizeof texto[8 + i], "%ld", (long)filhos[i]);
            args[3 + i] = texto[2 + i];
            args[9 + i] = texto[8 + i];
        }
        args[15] = NULL;
        close(canal_irq[1]);
        close(canal_pedidos[1]);
        /* Nao mistura a saida do kernel com o relatorio dos testes. */
        FILE *saida = fopen("/dev/null", "w");
        if (saida) {
            dup2(fileno(saida), STDOUT_FILENO);
            fclose(saida);
        }
        execv("./KernelSim", args);
        _exit(127);
    }
    close(canal_irq[0]);
    close(canal_pedidos[0]);
    for (int i = 0; i < 6; i++) close(respostas[i][1]);

    /* A ordem das solicitacoes determina a ordem de atendimento FIFO. */
    usleep(100000);
    if (!pedido(canal_pedidos[1], 1, ENVIAR)) goto limpar;
    usleep(100000);
    if (!pedido(canal_pedidos[1], 2, ENVIAR)) goto limpar;
    usleep(100000);
    if (!pedido(canal_pedidos[1], 3, RECEBER)) goto limpar;
    usleep(100000);
    if (!pedido(canal_pedidos[1], 4, RECEBER)) goto limpar;
    usleep(200000);

    for (int i = 0; i < 4; i++)
        if (disponivel(respostas[i][0], 20)) goto limpar;

    if (!irq(canal_irq[1], IRQ2) ||
        !resposta(respostas[0][0], 1, ENVIAR, 100) ||
        disponivel(respostas[1][0], 30)) goto limpar;
    if (!irq(canal_irq[1], IRQ2) ||
        !resposta(respostas[1][0], 2, ENVIAR, 200)) goto limpar;
    if (!irq(canal_irq[1], IRQ1) ||
        !resposta(respostas[2][0], 3, RECEBER, 0) ||
        disponivel(respostas[3][0], 30)) goto limpar;
    if (!irq(canal_irq[1], IRQ1) ||
        !resposta(respostas[3][0], 4, RECEBER, 0)) goto limpar;

    puts("[OK] Bloqueio e filas FIFO: SEND A1/A2 e RECV A3/A4.");
    resultado = 1;

limpar:
    if (kernel > 0) { kill(kernel, SIGKILL); waitpid(kernel, NULL, 0); }
    for (int i = 0; i < criados; i++) {
        kill(filhos[i], SIGKILL);
        waitpid(filhos[i], NULL, 0);
    }
    close(canal_irq[1]);
    close(canal_pedidos[1]);
    /* No caminho de erro, alguns descritores podem ainda estar abertos;
     * o sistema operacional os fechara ao terminar o processo de testes. */
    if (!resultado) puts("[ERRO] Bloqueio, filas ou interrupcoes.");
    return resultado;
}
