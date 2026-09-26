#include "testes.h"
#include "apoio.h"
#include <stdio.h>

int testar_filas(void) {
    AmbienteTeste a;
    int ok = 0;
    puts("\n[TesteFilas] Testando bloqueio e filas FIFO...");
    if (!iniciar_ambiente(&a)) return 0;

    /* Os pedidos entram na mesma pipe, que conserva a ordem de escrita. */
    if (!enviar_pedido_teste(&a, 1, ENVIAR, 11, 101) ||
        !enviar_pedido_teste(&a, 2, ENVIAR, 22, 202) ||
        !enviar_pedido_teste(&a, 3, RECEBER, 33, 303) ||
        !enviar_pedido_teste(&a, 4, RECEBER, 44, 404)) goto fim;

    if (!enviar_irq_teste(&a, IRQ2) ||
        !ler_resposta_teste(&a, 1, ENVIAR, 101) ||
        !enviar_irq_teste(&a, IRQ2) ||
        !ler_resposta_teste(&a, 2, ENVIAR, 202)) goto fim;
    puts("[OK] IRQ2 concluiu SEND A1 antes de SEND A2.");

    if (!enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 3, RECEBER, 0) ||
        !enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 4, RECEBER, 0)) goto fim;
    puts("[OK] IRQ1 concluiu RECV A3 antes de RECV A4.");
    ok = 1;
fim:
    encerrar_ambiente(&a);
    if (!ok) puts("[FALHOU] Bloqueio e filas FIFO.");
    return ok;
}
