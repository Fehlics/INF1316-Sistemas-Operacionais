#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 4) return 1;
    int id = atoi(argv[1]), controle = atoi(argv[2]), resposta = atoi(argv[3]);
    int pc = 0, n = 0, limite = MAX;
    char *teste = getenv("TESTE_MAX");
    if (teste && atoi(teste) > 0 && atoi(teste) <= MAX) limite = atoi(teste);
    int espera = getenv("TESTE_US") ? atoi(getenv("TESTE_US")) : 0;
    int modo = getenv("TESTE_OP") ? atoi(getenv("TESTE_OP")) : 0;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    while (pc < limite) {
        pc++;
        Mensagem m = { .tipo = CONTEXTO, .id = id, .pc = pc, .n = n };
        if (write(controle, &m, sizeof m) != sizeof m) return 1;
        if (espera > 0) usleep(espera);
        else sleep(1);

        int op = NENHUMA;
        if (modo && pc == 2) {
            if (modo == 1 && id <= 2) op = id == 1 ? ENVIAR : RECEBER;
            if (modo == 2) op = id % 2 ? ENVIAR : RECEBER;
            if (modo == 3) op = id % 2 ? RECEBER : ENVIAR;
            if (modo == 4 && id % 2 == 0) op = RECEBER;
        } else if (!modo && !getenv("TESTE_SEM_OP") && rand() % 100 < 15)
            op = rand() % 2 ? RECEBER : ENVIAR;

        if (op != NENHUMA) {
            m.tipo = PEDIDO;
            m.op = op;
            m.endereco = op == ENVIAR ? END_PC : END_N;
            if (write(controle, &m, sizeof m) != sizeof m) return 1;
            Resposta r;
            if (read(resposta, &r, sizeof r) != sizeof r || r.op != op) return 1;
            pc = r.pc;
            n = r.n;
            m.tipo = CONTEXTO;
            m.pc = pc;
            m.n = n;
            if (write(controle, &m, sizeof m) != sizeof m) return 1;
        }
    }
    Mensagem fim = { .tipo = FIM, .id = id, .pc = pc, .n = n };
    if (write(controle, &fim, sizeof fim) != sizeof fim) return 1;
    Resposta r;
    if (read(resposta, &r, sizeof r) != sizeof r || r.op != NENHUMA) return 1;
    printf("FINAL A%d PC=%d N=%d\n", id, pc, n);
    close(controle);
    close(resposta);
    return 0;
}
