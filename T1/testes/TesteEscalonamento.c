#include "testes.h"
#include "apoio.h"
#include <stdio.h>

/* Testa o Round Robin sem examinar arquivos especificos de /proc. */
static int avancar(AmbienteTeste *a, int proximo, int anterior) {
    return enviar_irq_teste(a, IRQ0) &&
           esperar_ativo(a, proximo) &&
           verificar_parada(a->auxiliares[anterior - 1]);
}

int testar_escalonamento(void) {
    AmbienteTeste a;
    int ok = 0;
    puts("\n[TesteEscalonamento] Iniciando IRQs controladas...");
    if (!iniciar_ambiente(&a)) return 0;

    /* KernelSim inicia em A1; cada IRQ0 segue para a proxima aplicacao. */
    for (int i = 2; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar(&a, i, i - 1)) goto fim;
    if (!avancar(&a, 1, 6)) goto fim;
    puts("[OK] Round Robin percorreu A1..A6 e retornou a A1.");
    puts("[OK] A aplicacao anterior foi interrompida por SIGSTOP.");

    /* Bloqueio por RECV deve escolher A2 imediatamente. */
    if (!enviar_pedido_teste(&a, 1, RECEBER, 23, 17) ||
        !esperar_ativo(&a, 2) ||
        !verificar_parada(a.auxiliares[0])) goto fim;
    puts("[OK] Processo bloqueado deixou a CPU imediatamente.");

    for (int i = 3; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar(&a, i, i - 1)) goto fim;
    if (!avancar(&a, 2, 6)) goto fim;
    puts("[OK] Processo bloqueado foi ignorado pelo Round Robin.");

    /* IRQ1 conclui a leitura e coloca A1 como PRONTO, sem preempcao. */
    if (!enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 1, RECEBER, 0)) goto fim;
    for (int i = 3; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar(&a, i, i - 1)) goto fim;
    if (!avancar(&a, 1, 6)) goto fim;
    puts("[OK] A1 desbloqueada aguardou sua vez para executar.");

    /* IRQ1/IRQ2 sem pedidos nao devem alterar quem ocupa a CPU. */
    if (!enviar_irq_teste(&a, IRQ1) ||
        !enviar_irq_teste(&a, IRQ2) ||
        !avancar(&a, 2, 1)) goto fim;
    puts("[OK] Interrupcoes sem pedidos nao afetaram a ordem.");
    ok = 1;
fim:
    encerrar_ambiente(&a);
    if (!ok) puts("[FALHOU] Escalonamento Round Robin.");
    return ok;
}
