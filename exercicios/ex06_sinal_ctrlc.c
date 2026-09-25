/* ex06_sinal_ctrlc.c  --  Lab 3 (exercicio 1, versao guiada)
 * Ctrl-C (SIGINT) e capturado e apenas conta; no 3o Ctrl-C o programa encerra sozinho.
 * Ctrl-\ (SIGQUIT) sai imediatamente.
 * Depois teste: comente os dois signal() e veja o comportamento padrao.
 * (Tambem cobre o exercicio 2: SIGKILL pode ser capturado?)
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t contador = 0;

static void trata_int(int sinal)
{
    (void)sinal;
    // TODO: Incremente 'contador' e avise com write(STDOUT_FILENO, ...).
    //       (write e "async-signal-safe"; printf NAO e, embora "funcione" nos slides)
    // >>> escreva seu codigo aqui <<<
}

static void trata_quit(int sinal)
{
    (void)sinal;
    static const char m[] = "\nSIGQUIT: terminando o processo...\n";
    if (write(STDOUT_FILENO, m, sizeof m - 1) < 0) { /* ignora */ }
    _exit(0);
}

int main(void)
{
    // TODO: Instale trata_int para SIGINT e trata_quit para SIGQUIT com signal().
    //       Compare o retorno com SIG_ERR e imprima o endereco do manipulador anterior (%p).
    // >>> escreva seu codigo aqui <<<

    // TODO: Extra (Lab 3, exercicio 2): tente instalar trata_int tambem para SIGKILL.
    //       Compare o retorno com SIG_ERR e mostre o motivo com perror("signal(SIGKILL)").
    // >>> escreva seu codigo aqui <<<

    printf("pid=%d. Ctrl-C conta; 3 Ctrl-C encerram; Ctrl-\\ sai na hora.\n", getpid());
    while (contador < 3)
        pause();               /* dorme ate chegar QUALQUER sinal tratado */
    printf("Recebi %d Ctrl-C. Tchau!\n", (int)contador);
    return 0;
}
