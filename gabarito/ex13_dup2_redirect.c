/* ex13_dup2_redirect.c  --  Lab 4 (exercicio 2)
 * Uso: ./ex13_dup2_redirect entrada.txt saida.txt
 * O programa so usa getchar/putchar (stdin/stdout); quem le/escreve nos arquivos e o
 * redirecionamento feito com dup2. Copia o texto em MAIUSCULAS.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 3) { fprintf(stderr, "uso: %s entrada saida\n", argv[0]); return 1; }

    /* [resolvido] 1) in  = open(argv[1], O_RDONLY);  out = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644) */
    int in = open(argv[1], O_RDONLY);
    if (in < 0) { perror("open entrada"); return 1; }
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { perror("open saida"); return 1; }
    if (dup2(in, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0) { perror("dup2"); return 1; }
    close(in);
    close(out);

    int c;
    while ((c = getchar()) != EOF)
        putchar(toupper(c));

    fprintf(stderr, "copiei %s -> %s (esta mensagem foi pelo stderr, que nao foi redirecionado)\n",
            argv[1], argv[2]);
    return 0;
}
