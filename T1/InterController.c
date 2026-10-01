#include "trabalho.h"
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2)
        return 1;

    int controle = atoi(argv[1]);
    while (1) {
        sleep(1);

        Mensagem m = {0};
        m.tipo = IRQ;
        m.op = NENHUMA;
        if (write(controle, &m, sizeof m) != sizeof m)
            break;

        if (getenv("TESTE_AUTO") != NULL || rand() % 100 < 10) {
            m.op = RECEBER;
            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }
        if (getenv("TESTE_AUTO") != NULL || rand() % 100 < 5) {
            m.op = ENVIAR;
            if (write(controle, &m, sizeof m) != sizeof m)
                break;
        }
    }
    close(controle);
    return 0;
}