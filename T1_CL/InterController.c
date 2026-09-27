#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int emitir(int fd, TipoIRQ irq) {
    MensagemControle m = {0};
    m.tipo = EVT_INTERRUPCAO;
    m.irq = irq;
    return write(fd, &m, sizeof m) == (ssize_t)sizeof m;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: InterController FD_CONTROLE\n");
        return 1;
    }
    int fd = atoi(argv[1]);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    for (;;) {
        usleep(500000); /* IRQ0 a cada 500ms */
        if (!emitir(fd, IRQ0)) break;
        if (rand() % 100 < 10 && !emitir(fd, IRQ1)) break; /* P1 = 0.10 */
        if (rand() % 100 < 5 && !emitir(fd, IRQ2)) break;  /* P2 = 0.05 */
    }
    close(fd);
    return 0;
}
