#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* O mesmo canal serve para as interrupcoes e os pedidos das aplicacoes. */
static int emitir(int fd, TipoIRQ irq) {
    MensagemControle mensagem = {0};
    mensagem.tipo = EVENTO_INTERRUPCAO;
    mensagem.irq = irq;
    return write(fd, &mensagem, sizeof mensagem) == (ssize_t)sizeof mensagem;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: InterController FD_CONTROLE\n");
        return 1;
    }
    int fd_controle = atoi(argv[1]);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    for (;;) {
        /* usleep: versao de sleep que permite o meio segundo exigido. */
        usleep(500000);
        if (!emitir(fd_controle, IRQ0)) break;
        if (rand() % 100 < 10 && !emitir(fd_controle, IRQ1)) break;
        if (rand() % 100 < 5 && !emitir(fd_controle, IRQ2)) break;
    }
    close(fd_controle);
    return 0;
}
