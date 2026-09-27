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
    /* Apenas em teste: permite concluir syscalls sem esperar minutos.
     * Na execucao normal permanece o IRQ0 original a cada 500 ms. */
    int intervalo = 500000;
    const char *texto_intervalo = getenv("TESTE_INTERVALO_IRQ_US");
    if (texto_intervalo != NULL) {
        char *fim;
        long valor = strtol(texto_intervalo, &fim, 10);
        if (*fim == '\0' && valor >= 1000 && valor <= 500000)
            intervalo = (int)valor;
    }
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    for (;;) {
        /* usleep: versao de sleep que permite o meio segundo exigido. */
        usleep((useconds_t)intervalo);
        if (!emitir(fd_controle, IRQ0)) break;
        if (rand() % 100 < 10 && !emitir(fd_controle, IRQ1)) break;
        if (rand() % 100 < 5 && !emitir(fd_controle, IRQ2)) break;
    }
    close(fd_controle);
    return 0;
}
