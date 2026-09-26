#include "trabalho.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Envia uma interrupção ao KernelSim por um pipe Unix real. */
static int enviar_irq(int fd, TipoIRQ tipo) {
    MensagemIRQ mensagem = { .tipo = tipo };
    return write(fd, &mensagem, sizeof mensagem) == (ssize_t)sizeof mensagem;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: InterController FD_IRQ\n");
        return 1;
    }

    int fd_irq = atoi(argv[1]);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    struct timespec meio_segundo = {0, 500000000L};

    for (;;) {
        /* sleep(0.5) não funciona: sleep recebe segundos inteiros. */
        nanosleep(&meio_segundo, NULL);

        if (!enviar_irq(fd_irq, IRQ0)) break;      /* A cada 500 ms. */
        if (rand() % 100 < 10) {                   /* Probabilidade 10%. */
            if (!enviar_irq(fd_irq, IRQ1)) break;
        }
        if (rand() % 100 < 5) {                    /* Probabilidade 5%. */
            if (!enviar_irq(fd_irq, IRQ2)) break;
        }
    }

    close(fd_irq);
    return 0;
}
