#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    int controle = atoi(argv[1]);
    int espera = getenv("TESTE_IRQ_US") ? atoi(getenv("TESTE_IRQ_US")) : 500000;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    for (;;) {
        usleep(espera);
        Mensagem m = { .tipo = IRQ, .op = 0 };
        if (write(controle, &m, sizeof m) != sizeof m) break;
        if (rand() % 100 < 10) {
            m.op = 1;
            if (write(controle, &m, sizeof m) != sizeof m) break;
        }
        if (rand() % 100 < 5) {
            m.op = 2;
            if (write(controle, &m, sizeof m) != sizeof m) break;
        }
    }
    close(controle);
    return 0;
}
