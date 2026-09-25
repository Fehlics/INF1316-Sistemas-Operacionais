/* ex09_sigfpe.c  --  Lab 3 (exercicio 5)
 * Le 2 inteiros e mostra + - * /.
 *   1) rode com "10 0" SEM o handler (comente o signal): "Floating point exception"
 *   2) com o handler: mensagem propria.
 * Por que o handler chama _exit() e nao simplesmente retorna? Descubra (dica: qual
 * instrucao esta sendo re-executada quando o handler retorna?).
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void trata_fpe(int sinal)
{
    (void)sinal;
    /* [resolvido] Escreva (write) uma mensagem de "divisao por zero" e termine com _exit(1). */
    static const char m[] = "\nErro: operacao aritmetica invalida (SIGFPE capturado)\n";
    if (write(STDERR_FILENO, m, sizeof m - 1) < 0) { /* ignora */ }
    _exit(1);
}

int main(void)
{
    int a, b;
    /* [resolvido] Instale trata_fpe para SIGFPE. */
    signal(SIGFPE, trata_fpe);

    printf("Digite dois inteiros: ");
    if (scanf("%d %d", &a, &b) != 2) return 1;
    printf("%d + %d = %d\n", a, b, a + b);
    printf("%d - %d = %d\n", a, b, a - b);
    printf("%d * %d = %d\n", a, b, a * b);
    fflush(stdout);
    printf("%d / %d = %d\n", a, b, a / b);
    return 0;
}
