#include <stdio.h>
#include "testes.h"

/* Programa principal dos testes: novos testes serao adicionados aqui. */
int main(void) {
    int aprovados = 0;
    int total = 1;

    puts("=== TESTES DO SIMULADOR ===");

    if (testar_processos()) {
        aprovados++;
        puts("[PASSOU] Criacao e encerramento dos processos");
    } else {
        puts("[FALHOU] Criacao e encerramento dos processos");
    }

    printf("\nResultado: %d de %d testes passaram.\n", aprovados, total);
    return aprovados == total ? 0 : 1;
}
