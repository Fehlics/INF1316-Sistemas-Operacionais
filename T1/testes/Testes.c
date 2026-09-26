#include <stdio.h>
#include "testes.h"

/* Executa todos os testes e informa quantos passaram. */
int main(void) {
    int aprovados = 0;
    int total = 3;
    puts("=== TESTES DO SIMULADOR ===");

    if (testar_processos()) {
        aprovados++;
        puts("[PASSOU] Criacao e encerramento dos processos");
    } else {
        puts("[FALHOU] Criacao e encerramento dos processos");
    }

    if (testar_filas()) {
        aprovados++;
        puts("[PASSOU] Bloqueio, filas FIFO e desbloqueio");
    } else {
        puts("[FALHOU] Bloqueio, filas FIFO e desbloqueio");
    }

    if (testar_respostas()) {
        aprovados++;
        puts("[PASSOU] Aplicacao aguarda resposta da syscall");
    } else {
        puts("[FALHOU] Aplicacao aguarda resposta da syscall");
    }

    printf("\nResultado: %d de %d testes passaram.\n", aprovados, total);
    return aprovados == total ? 0 : 1;
}
