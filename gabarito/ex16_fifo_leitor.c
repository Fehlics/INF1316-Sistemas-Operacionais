/* ex16_fifo_leitor.c  --  Lab 6 (exercicio 1, parte 1): loop lendo da FIFO e escrevendo na tela */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO "minhaFifo"

int main(void)
{
    if (mkfifo(FIFO, S_IRUSR | S_IWUSR) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }

    char buf[256];
    for (;;) {
        /* [resolvido] Abra a FIFO com O_RDONLY (bloqueia ate um escritor abrir). Leia com read() ate */
        int fd = open(FIFO, O_RDONLY);
        if (fd < 0) { perror("open"); return 1; }
        ssize_t n;
        while ((n = read(fd, buf, sizeof buf)) > 0)
            if (write(STDOUT_FILENO, buf, (size_t)n) < 0) break;
        close(fd);
        puts("[leitor] escritor saiu; esperando o proximo...");
        fflush(stdout);
    }
}
