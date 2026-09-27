#include "trabalho.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int enviar(int fd, MensagemControle *m) {
    ssize_t r;
    do { r = write(fd, m, sizeof *m); } while (r < 0 && errno == EINTR);
    return r == (ssize_t)sizeof *m;
}

static int receber(int fd, RespostaSyscall *r) {
    size_t total = 0;
    while (total < sizeof *r) {
        ssize_t n = read(fd, (char *)r + total, sizeof *r - total);
        if (n == 0) return 0;
        if (n < 0) { if (errno == EINTR) continue; return 0; }
        total += (size_t)n;
    }
    return 1;
}

/* Informa PC/N atuais ao KernelSim (mantem o PCB simulado em dia). */
static int informar_contexto(int controle, int id, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVT_CONTEXTO;
    m.dado.id = id;
    m.dado.pc = pc;
    m.dado.n = n;
    return enviar(controle, &m);
}

/* Faz a syscall (send/recv) e espera a resposta do kernel. */
static int syscall_pipe(int controle, int resp, int id, Operacao op,
                       int *pc, int *n) {
    MensagemControle m = {0};
    m.tipo = EVT_SYSCALL;
    m.dado.id = id;
    m.dado.operacao = op;
    m.dado.pc = *pc;
    m.dado.n = *n;
    if (!enviar(controle, &m)) return 0;
    printf("[A%d] Solicitou %s (PC=%d, N=%d)\n", id,
           op == ENVIAR ? "SEND" : "RECV", *pc, *n);

    RespostaSyscall r;
    if (!receber(resp, &r)) return 0;
    *pc = r.pc;
    *n = r.n;
    printf("[A%d] %s concluido (PC=%d, N=%d)\n", id,
           op == ENVIAR ? "SEND" : "RECV", *pc, *n);
    return 1;
}

static int avisar_termino(int controle, int resp, int id, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVT_TERMINO;
    m.dado.id = id;
    m.dado.pc = pc;
    m.dado.n = n;
    if (!enviar(controle, &m)) return 0;
    RespostaSyscall r;
    return receber(resp, &r);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: Application ID FD_CONTROLE FD_RESPOSTA\n");
        return 1;
    }
    int id = atoi(argv[1]);
    int controle = atoi(argv[2]);
    int resp = atoi(argv[3]);
    int pc = 0, n = 0;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < MAX_ITERACOES) {
        pc++;
        if (!informar_contexto(controle, id, pc, n)) return 1;
        sleep(1);

        if (rand() % 100 < 15) { /* baixa probabilidade de syscall */
            Operacao op = rand() % 2 == 0 ? ENVIAR : RECEBER;
            if (!syscall_pipe(controle, resp, id, op, &pc, &n)) return 1;
            if (!informar_contexto(controle, id, pc, n)) return 1;
        }
    }
    printf("[A%d] Finalizada (PC=%d, N=%d).\n", id, pc, n);
    return avisar_termino(controle, resp, id, pc, n) ? 0 : 1;
}
