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
    /* [resolvido] Incremente 'contador' e avise com write(STDOUT_FILENO, ...). */
    contador++;
    static const char m[] = "\n[SIGINT capturado]\n";
    if (write(STDOUT_FILENO, m, sizeof m - 1) < 0) { /* ignora */ }
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
    /* [resolvido] Instale trata_int para SIGINT e trata_quit para SIGQUIT com signal(). */
    void (*ant)(int);
    ant = signal(SIGINT, trata_int);
    if (ant == SIG_ERR) { perror("signal SIGINT"); return 1; }
    printf("manipulador anterior de SIGINT: %p (SIG_DFL=%p)\n", (void *)ant, (void *)SIG_DFL);
    if (signal(SIGQUIT, trata_quit) == SIG_ERR) { perror("signal SIGQUIT"); return 1; }

    /* [resolvido] Extra (Lab 3, exercicio 2): tente instalar trata_int tambem para SIGKILL. */
    if (signal(SIGKILL, trata_int) == SIG_ERR)
        perror("signal(SIGKILL)");

    printf("pid=%d. Ctrl-C conta; 3 Ctrl-C encerram; Ctrl-\\ sai na hora.\n", getpid());
    while (contador < 3)
        pause();               /* dorme ate chegar QUALQUER sinal tratado */
    printf("Recebi %d Ctrl-C. Tchau!\n", (int)contador);
    return 0;
}
