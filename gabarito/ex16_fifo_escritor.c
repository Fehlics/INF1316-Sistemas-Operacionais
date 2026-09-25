/* ex16_fifo_escritor.c  --  Lab 6 (exercicio 1, parte 2)
 * Terminal 2: le do teclado e escreve na FIFO.  Terminal 1: ./ex16_fifo_leitor
 * Experimente: inicie so um dos lados e observe o open() bloqueando.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO "minhaFifo"

int main(void)
{
    /* [resolvido] 1) mkfifo(FIFO, S_IRUSR | S_IWUSR); aceite o erro EEXIST (a FIFO ja existir e ok). */
    if (mkfifo(FIFO, S_IRUSR | S_IWUSR) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }

    int fd;
    puts("abrindo FIFO para escrita (bloqueia ate haver um leitor)...");
    /* [resolvido] 2) fd = open(FIFO, O_WRONLY)  -- NAO use "w": open() recebe flags inteiras. */
    fd = open(FIFO, O_WRONLY);
    if (fd < 0) { perror("open"); return 1; }
    puts("leitor conectado. Digite linhas (Ctrl-D encerra):");

    char linha[256];
    while (fgets(linha, sizeof linha, stdin)) {
        /* [resolvido] 3) write(fd, linha, strlen(linha)); */
        write(fd, linha, strlen(linha));
    }
    close(fd);
    return 0;
}
