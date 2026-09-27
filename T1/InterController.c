#include "trabalho.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;

    int controle = atoi(argv[1]);
    int teste = getenv("TESTE_AUTO") != NULL;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    for (;;) {
        /* sleep trabalha com segundos; o enunciado permite mudar o intervalo. */
        sleep(1);

        Mensagem m = { .tipo = IRQ, .op = NENHUMA };
        if (write(controle, &m, sizeof m) != sizeof m)
            break;

        /* Nos testes, concluimos os pedidos a cada ciclo, sem sorteio. */
        if (teste || rand() % 100 < 10) {
            m.op = RECEBER;
            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }

        if (teste || rand() % 100 < 5) {
            m.op = ENVIAR;
            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }
    }

    close(controle);
    return 0;
}
