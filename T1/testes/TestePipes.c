#include "testes.h"
#include "apoio.h"
#include <stdio.h>

static int escrever(AmbienteTeste *a, int id, int pc) {
    return enviar_pedido_teste(a, id, ENVIAR, pc, id * 10) &&
           enviar_irq_teste(a, IRQ2) &&
           ler_resposta_teste(a, id, ENVIAR, id * 10);
}
static int receber(AmbienteTeste *a, int id, int n) {
    return enviar_pedido_teste(a, id, RECEBER, 100 + id, id * 10) &&
           enviar_irq_teste(a, IRQ1) &&
           ler_resposta_teste(a, id, RECEBER, n);
}

int testar_pipes(void) {
    AmbienteTeste a;
    int ok = 0;
    puts("\n[TestePipes] Testando os seis buffers dos tres pares...");
    if (!iniciar_ambiente(&a)) return 0;

    if (!receber(&a, 2, 0)) goto fim;
    puts("[OK] Leitura sem mensagens devolve N=0.");

    if (!escrever(&a, 1, 11) || !escrever(&a, 1, 12) ||
        !escrever(&a, 3, 31) || !escrever(&a, 5, 51) ||
        !escrever(&a, 2, 21)) goto fim;
    puts("[OK] Escritas entre os tres pares nos dois sentidos.");

    /* Ambas as leituras aguardam IRQ1, independentemente de seu pipe. */
    if (!enviar_pedido_teste(&a, 4, RECEBER, 104, 40) ||
        !enviar_pedido_teste(&a, 6, RECEBER, 106, 60) ||
        !enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 4, RECEBER, 31) ||
        !enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 6, RECEBER, 51)) goto fim;
    puts("[OK] Leituras de pipes diferentes respeitam a fila global.");

    if (!receber(&a, 2, 11) || !receber(&a, 2, 12) ||
        !receber(&a, 1, 21) || !receber(&a, 2, 0) ||
        !receber(&a, 4, 0) || !receber(&a, 6, 0)) goto fim;
    puts("[OK] Buffers FIFO, bidirecionais e independentes.");

    /* SEND so insere o contador quando recebe IRQ2. */
    if (!enviar_pedido_teste(&a, 1, ENVIAR, 55, 10) ||
        !receber(&a, 2, 0) || !enviar_irq_teste(&a, IRQ2) ||
        !ler_resposta_teste(&a, 1, ENVIAR, 10) ||
        !receber(&a, 2, 55)) goto fim;
    puts("[OK] O valor do SEND so aparece apos a IRQ2.");
    ok = 1;
fim:
    encerrar_ambiente(&a);
    if (!ok) puts("[FALHOU] Teste dos pipes simulados.");
    return ok;
}
