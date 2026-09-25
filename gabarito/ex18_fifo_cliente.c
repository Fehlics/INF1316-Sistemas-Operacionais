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
    /* [resolvido] Abra REQ com O_WRONLY e depois RESP com O_RDONLY (mesma ordem que o servidor espera). */
    fdreq = open(REQ, O_WRONLY);
    if (fdreq < 0) { perror("open req (o servidor esta rodando?)"); return 1; }
    fdresp = open(RESP, O_RDONLY);
    if (fdresp < 0) { perror("open resp"); return 1; }

    char linha[256], resp[256];
    while (printf("> "), fflush(stdout), fgets(linha, sizeof linha, stdin)) {
        /* [resolvido] write(fdreq, linha, strlen(linha)); depois n = read(fdresp, resp, sizeof resp - 1); */
        write(fdreq, linha, strlen(linha));
        ssize_t n = read(fdresp, resp, sizeof resp - 1);
        if (n <= 0) break;
        resp[n] = '\0';
        printf("resposta: %s", resp);
    }
    close(fdreq);
    close(fdresp);
    return 0;
}
