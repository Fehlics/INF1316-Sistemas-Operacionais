#include "trabalho.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4)
        return 1;

    int id = atoi(argv[1]);
    int controle = atoi(argv[2]);
    int resposta = atoi(argv[3]);
    int pc = 0;
    int n = 0;
    int limite = MAX;
    int modo = 0;

    if (getenv("TESTE_MAX") != NULL)
        limite = atoi(getenv("TESTE_MAX"));
    if (getenv("TESTE_OP") != NULL)
        modo = atoi(getenv("TESTE_OP"));

    while (pc < limite) {
        pc++;
        Mensagem m = {0};
        m.tipo = CONTEXTO;
        m.id = id;
        m.pc = pc;
        m.n = n;
        if (write(controle, &m, sizeof m) != sizeof m)
            return 1;

        if (getenv("TESTE_RAPIDO") == NULL)
            sleep(1);
        else if (id == 1 && (pc == 1 || pc == limite / 2))
            sleep(1);

        int op = NENHUMA;
        if (modo > 0 && pc == 2) {
            if (modo == 1 && id % 2 == 1)
                op = ENVIAR;
            if (modo == 1 && id % 2 == 0)
                op = RECEBER;
            if (modo == 2 && id % 2 == 0)
                op = ENVIAR;
            if (modo == 2 && id % 2 == 1)
                op = RECEBER;
            if (modo == 3 && id % 2 == 0)
                op = RECEBER;
        }
        else if (modo == 0 && getenv("TESTE_RAPIDO") == NULL) {
            if (rand() % 100 < 15) {
                if (rand() % 2 == 0)
                    op = ENVIAR;
                else
                    op = RECEBER;
            }
        }

        if (op != NENHUMA) {
            m.tipo = PEDIDO;
            m.op = op;
            if (op == ENVIAR)
                m.endereco = END_PC;
            else
                m.endereco = END_N;
            if (write(controle, &m, sizeof m) != sizeof m)
                return 1;

            Resposta r;
            if (read(resposta, &r, sizeof r) != sizeof r)
                return 1;
            if (r.op != op || r.pc != pc)
                return 1;
            if (modo == 3 && r.n != 0)
                return 1;
            pc = r.pc;
            n = r.n;

            m.tipo = CONTEXTO;
            m.pc = pc;
            m.n = n;
            if (write(controle, &m, sizeof m) != sizeof m)
                return 1;
        }
    }

    Mensagem fim = {0};
    fim.tipo = FIM;
    fim.id = id;
    fim.pc = pc;
    fim.n = n;
    if (write(controle, &fim, sizeof fim) != sizeof fim)
        return 1;

    Resposta r;
    if (read(resposta, &r, sizeof r) != sizeof r || r.op != NENHUMA)
        return 1;
    printf("FINAL A%d PC=%d N=%d\n", id, pc, n);
    close(controle);
    close(resposta);
    return 0;
}