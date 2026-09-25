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
    // TODO: 1) mkfifo(FIFO, S_IRUSR | S_IWUSR); aceite o erro EEXIST (a FIFO ja existir e ok).
    // >>> escreva seu codigo aqui <<<

    int fd;
    puts("abrindo FIFO para escrita (bloqueia ate haver um leitor)...");
    // TODO: 2) fd = open(FIFO, O_WRONLY)  -- NAO use "w": open() recebe flags inteiras.
    // >>> escreva seu codigo aqui <<<
    puts("leitor conectado. Digite linhas (Ctrl-D encerra):");

    char linha[256];
    while (fgets(linha, sizeof linha, stdin)) {
        // TODO: 3) write(fd, linha, strlen(linha));
        // >>> escreva seu codigo aqui <<<
    }
    close(fd);
    return 0;
}
