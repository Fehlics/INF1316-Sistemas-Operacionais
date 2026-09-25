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
    // TODO: Escreva (write) uma mensagem de "divisao por zero" e termine com _exit(1).
    // >>> escreva seu codigo aqui <<<
}

int main(void)
{
    int a, b;
    // TODO: Instale trata_fpe para SIGFPE.
    // >>> escreva seu codigo aqui <<<

    printf("Digite dois inteiros: ");
    if (scanf("%d %d", &a, &b) != 2) return 1;
    printf("%d + %d = %d\n", a, b, a + b);
    printf("%d - %d = %d\n", a, b, a - b);
    printf("%d * %d = %d\n", a, b, a * b);
    fflush(stdout);
    printf("%d / %d = %d\n", a, b, a / b);
    return 0;
}
