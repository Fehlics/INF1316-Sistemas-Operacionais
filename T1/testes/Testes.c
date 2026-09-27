#include <stdio.h>
#include "testes.h"

/* Executa todos os testes e informa quantos passaram. */
int main(void) {
    int aprovados = 0;
    int total = 8;
    puts("=== TESTES DO SIMULADOR ===");

    if (testar_processos()) {
        aprovados++;
        puts("[PASSOU] Criacao e encerramento dos processos");
    } else {
        puts("[FALHOU] Criacao e encerramento dos processos");
    }

    if (testar_escalonamento()) {
        aprovados++;
        puts("[PASSOU] Escalonamento Round Robin");
    } else {
        puts("[FALHOU] Escalonamento Round Robin");
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

    if (testar_termino()) {
        aprovados++;
        puts("[PASSOU] Reconhecimento do termino dos processos");
    } else {
        puts("[FALHOU] Reconhecimento do termino dos processos");
    }

    if (testar_pipes()) {
        aprovados++;
        puts("[PASSOU] Seis buffers dos pipes simulados");
    } else {
        puts("[FALHOU] Seis buffers dos pipes simulados");
    }

    if (testar_pausa()) {
        aprovados++;
        puts("[PASSOU] Contexto, pausa e retomada do simulador");
    } else {
        puts("[FALHOU] Contexto, pausa e retomada do simulador");
    }

    if (testar_contexto()) {
        aprovados++;
        puts("[PASSOU] Salvamento e retomada do contexto");
    } else {
        puts("[FALHOU] Salvamento e retomada do contexto");
    }

    printf("\nResultado: %d de %d testes passaram.\n", aprovados, total);
    return aprovados == total ? 0 : 1;
}
