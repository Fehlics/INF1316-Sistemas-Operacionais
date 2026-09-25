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
        // TODO: Abra a FIFO com O_RDONLY (bloqueia ate um escritor abrir). Leia com read() ate
        //       retornar 0 (EOF = todos os escritores fecharam) escrevendo na tela com
        //       write(STDOUT_FILENO, buf, n). Feche e volte ao topo do for(;;) para aceitar novo escritor.
        // >>> escreva seu codigo aqui <<<
    }
}
