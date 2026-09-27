#ifndef KERNEL_FILAS_H
#define KERNEL_FILAS_H

#include "trabalho.h"

typedef struct {
    int indices[QUANTIDADE_APLICACOES];
    int inicio, quantidade;
} FilaBloqueados;

int enfileirar(FilaBloqueados *fila, int indice);
int desenfileirar(FilaBloqueados *fila);
int parceiro(int indice);
int guardar(int remetente, int pc);
int retirar(int destinatario);

#endif
