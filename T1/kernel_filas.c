#include "kernel_filas.h"

/* Uma fila por operacao no kernel e um buffer por sentido de comunicacao. */
typedef struct {
    int dados[MAX_ITERACOES];
    int inicio, quantidade;
} BufferPipe;

static BufferPipe buffers[QUANTIDADE_APLICACOES];

int parceiro(int indice) {
    return indice % 2 == 0 ? indice + 1 : indice - 1;
}

int guardar(int remetente, int pc) {
    BufferPipe *b = &buffers[remetente];
    if (b->quantidade == MAX_ITERACOES) return 0;
    b->dados[(b->inicio + b->quantidade) % MAX_ITERACOES] = pc;
    b->quantidade++;
    return 1;
}

int retirar(int destinatario) {
    BufferPipe *b = &buffers[parceiro(destinatario)];
    if (!b->quantidade) return 0;
    int valor = b->dados[b->inicio];
    b->inicio = (b->inicio + 1) % MAX_ITERACOES;
    b->quantidade--;
    return valor;
}

int enfileirar(FilaBloqueados *f, int indice) {
    if (f->quantidade == QUANTIDADE_APLICACOES) return 0;
    f->indices[(f->inicio + f->quantidade) % QUANTIDADE_APLICACOES] = indice;
    f->quantidade++;
    return 1;
}

int desenfileirar(FilaBloqueados *f) {
    int indice = f->indices[f->inicio];
    f->inicio = (f->inicio + 1) % QUANTIDADE_APLICACOES;
    f->quantidade--;
    return indice;
}
