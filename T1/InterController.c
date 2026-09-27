#include "trabalho.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int emitir(int fd, TipoIRQ irq) {
    MensagemControle mensagem = {.tipo = EVENTO_INTERRUPCAO, .irq = irq};
    return enviar_dados(fd, &mensagem, sizeof mensagem);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: InterController FD_CONTROLE\n");
        return 1;
    }
    int controle = atoi(argv[1]);
    int intervalo = intervalo_teste("TESTE_INTERVALO_IRQ_US", 500000, 1000);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    for (;;) {
        usleep((useconds_t)intervalo);
        if (!emitir(controle, IRQ0)) break;
        if (rand() % 100 < 10 && !emitir(controle, IRQ1)) break;
        if (rand() % 100 < 5 && !emitir(controle, IRQ2)) break;
    }
    close(controle);
    return 0;
}
