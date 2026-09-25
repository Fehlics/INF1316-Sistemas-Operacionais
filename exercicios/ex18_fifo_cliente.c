/* ex18_fifo_cliente.c -- par do ex18_fifo_servidor.c
 * Le linhas do teclado, manda ao servidor e imprime a resposta em MAIUSCULAS. Ctrl-D sai.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define REQ  "fifo_req"
#define RESP "fifo_resp"

int main(void)
{
    int fdreq, fdresp;
    // TODO: Abra REQ com O_WRONLY e depois RESP com O_RDONLY (mesma ordem que o servidor espera).
    // >>> escreva seu codigo aqui <<<

    char linha[256], resp[256];
    while (printf("> "), fflush(stdout), fgets(linha, sizeof linha, stdin)) {
        // TODO: write(fdreq, linha, strlen(linha)); depois n = read(fdresp, resp, sizeof resp - 1);
        //       termine resp com '\0' e imprima "resposta: ...".
        // >>> escreva seu codigo aqui <<<
    }
    close(fdreq);
    close(fdresp);
    return 0;
}
