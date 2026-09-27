/* Testes simples do KernelSim: cria o kernel real com 6 processos
 * auxiliares (que apenas dormem, esperando SIGSTOP/SIGCONT) e envia
 * eventos "a mao" pela pipe de controle, como fariam Application e
 * InterController. Nao usa variaveis de ambiente nem infraestrutura
 * extra: só pipes, fork/exec e sinais. */
#include "../trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    int controle, estado, resposta[QUANTIDADE_APLICACOES];
    pid_t aux[QUANTIDADE_APLICACOES], kernel;
} Ambiente;

static int iniciar(Ambiente *a) {
    int ctrl[2], est[2], resp[QUANTIDADE_APLICACOES][2];
    pipe(ctrl); pipe(est);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) pipe(resp[i]);

    char pids[QUANTIDADE_APLICACOES][32], fdresp[QUANTIDADE_APLICACOES][32];
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pid_t pid = fork();
        if (pid == 0) { pause(); _exit(0); } /* auxiliar so aguarda sinais */
        a->aux[i] = pid;
        snprintf(pids[i], sizeof pids[i], "%ld", (long)pid);
        snprintf(fdresp[i], sizeof fdresp[i], "%d", resp[i][1]);
        a->resposta[i] = resp[i][0];
    }
    char fdctrl[32], fdest[32];
    snprintf(fdctrl, sizeof fdctrl, "%d", ctrl[0]);
    snprintf(fdest, sizeof fdest, "%d", est[1]);

    pid_t kernel = fork();
    if (kernel == 0) {
        execl("../KernelSim", "KernelSim", fdctrl,
              fdresp[0], fdresp[1], fdresp[2], fdresp[3], fdresp[4], fdresp[5],
              pids[0], pids[1], pids[2], pids[3], pids[4], pids[5], fdest, NULL);
        _exit(1);
    }
    a->kernel = kernel;
    a->controle = ctrl[1];
    a->estado = est[0];
    return 1;
}

static void encerrar(Ambiente *a) {
    kill(a->kernel, SIGKILL);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) kill(a->aux[i], SIGKILL);
    waitpid(a->kernel, NULL, 0);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) waitpid(a->aux[i], NULL, 0);
}

static void enviar(Ambiente *a, MensagemControle *m) {
    write(a->controle, m, sizeof *m);
    usleep(50000); /* tempo para o kernel processar antes do proximo passo */
}

static void irq(Ambiente *a, TipoIRQ tipo) {
    MensagemControle m = {0};
    m.tipo = EVT_INTERRUPCAO;
    m.irq = tipo;
    enviar(a, &m);
}

static void pedir_syscall(Ambiente *a, int id, Operacao op, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVT_SYSCALL;
    m.dado.id = id; m.dado.operacao = op; m.dado.pc = pc; m.dado.n = n;
    enviar(a, &m);
}

static int parado(pid_t pid) {
    char caminho[64];
    snprintf(caminho, sizeof caminho, "/proc/%d/stat", pid);
    FILE *f = fopen(caminho, "r");
    if (!f) return 0;
    char linha[256];
    fgets(linha, sizeof linha, f);
    fclose(f);
    char *fecha = strrchr(linha, ')');
    return fecha && fecha[2] == 'T';
}

static int esperar_resposta(int fd, int id, int n) {
    RespostaSyscall r;
    if (read(fd, &r, sizeof r) != (ssize_t)sizeof r) return 0;
    return r.id == id && r.n == n;
}

/* 1) Round Robin: cada IRQ0 deve ativar o proximo processo. */
static int teste_escalonamento(void) {
    Ambiente a;
    iniciar(&a);
    usleep(100000);
    int ok = 1;
    for (int i = 2; i <= QUANTIDADE_APLICACOES; i++) {
        irq(&a, IRQ0);
        if (!parado(a.aux[i - 2])) ok = 0; /* o anterior deve estar parado */
    }
    encerrar(&a);
    return ok;
}

/* 2) Bloqueio em fila e buffers: A1 envia, A2 recebe o valor de A1. */
static int teste_bloqueio_e_buffer(void) {
    Ambiente a;
    iniciar(&a);
    usleep(100000);
    pedir_syscall(&a, 1, ENVIAR, 42, 0);
    irq(&a, IRQ2); /* conclui o SEND de A1 */
    int ok = esperar_resposta(a.resposta[0], 1, 0);
    pedir_syscall(&a, 2, RECEBER, 7, 0);
    irq(&a, IRQ1); /* conclui o RECV de A2, que deve receber 42 */
    ok = ok && esperar_resposta(a.resposta[1], 2, 42);
    encerrar(&a);
    return ok;
}

/* 3) Termino: apos avisar o kernel, o processo sai do escalonamento. */
static int teste_termino(void) {
    Ambiente a;
    iniciar(&a);
    usleep(100000);
    MensagemControle m = {0};
    m.tipo = EVT_TERMINO;
    m.dado.id = 1; m.dado.pc = 5000; m.dado.n = 0;
    enviar(&a, &m);
    int ok = esperar_resposta(a.resposta[0], 1, 0);
    encerrar(&a);
    return ok;
}

/* 4) Pausa/retomada: o kernel deve responder com a fotografia dos PCBs. */
static int teste_pausa(void) {
    Ambiente a;
    iniciar(&a);
    usleep(100000);
    MensagemControle m = {0};
    m.tipo = EVT_PAUSAR;
    enviar(&a, &m);
    EstadoSimulador estado;
    int ok = read(a.estado, &estado, sizeof estado) == (ssize_t)sizeof estado;
    ok = ok && estado.executando == 1; /* A1 estava executando */
    m.tipo = EVT_RETOMAR;
    enviar(&a, &m);
    encerrar(&a);
    return ok;
}

int main(void) {
    struct { const char *nome; int (*fn)(void); } testes[] = {
        {"Escalonamento Round Robin", teste_escalonamento},
        {"Bloqueio, fila e buffer de pipe", teste_bloqueio_e_buffer},
        {"Reconhecimento de termino", teste_termino},
        {"Pausa e fotografia do estado", teste_pausa},
    };
    int aprovados = 0, total = (int)(sizeof testes / sizeof testes[0]);
    for (int i = 0; i < total; i++) {
        int ok = testes[i].fn();
        printf("[%s] %s\n", ok ? "PASSOU" : "FALHOU", testes[i].nome);
        aprovados += ok;
    }
    printf("\nResultado: %d de %d testes passaram.\n", aprovados, total);
    return aprovados == total ? 0 : 1;
}
