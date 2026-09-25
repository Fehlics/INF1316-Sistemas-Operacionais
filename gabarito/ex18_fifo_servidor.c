/* ex18_fifo_servidor.c  --  Lab 6 (exercicio 3)
 * Servidor: le palavras/linhas da FIFO de REQUISICAO e devolve em MAIUSCULAS pela FIFO
 * de RESPOSTA. Rode em background:  ./ex18_fifo_servidor &
 * Depois, em terminais diferentes:  ./ex18_fifo_cliente
 * Limitacao (pense nisso!): a FIFO de resposta e UNICA -> com varios clientes ao mesmo
 * tempo, a resposta pode ir para o cliente errado.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#define REQ  "fifo_req"
#define RESP "fifo_resp"

int main(void)
{
    signal(SIGPIPE, SIG_IGN);   /* cliente que some nao pode derrubar o servidor */

    /* [resolvido] Crie as duas FIFOs com mkfifo (aceite EEXIST). */
    if (mkfifo(REQ, 0600) < 0 && errno != EEXIST) { perror("mkfifo req"); return 1; }
    if (mkfifo(RESP, 0600) < 0 && errno != EEXIST) { perror("mkfifo resp"); return 1; }

    for (;;) {                       /* uma iteracao = uma "sessao" de cliente */
        int fdreq, fdresp;
        /* [resolvido] Abra REQ com O_RDONLY e DEPOIS RESP com O_WRONLY (nessa ordem! o cliente abre */
        fdreq = open(REQ, O_RDONLY);
        if (fdreq < 0) { perror("open req"); return 1; }
        fdresp = open(RESP, O_WRONLY);
        if (fdresp < 0) { perror("open resp"); return 1; }

        char buf[256];
        ssize_t n;
        /* [resolvido] Enquanto read(fdreq, buf, sizeof buf) > 0: converta cada byte com toupper() e */
        while ((n = read(fdreq, buf, sizeof buf)) > 0) {
            for (ssize_t i = 0; i < n; i++) buf[i] = (char)toupper((unsigned char)buf[i]);
            write(fdresp, buf, (size_t)n);
        }

        close(fdreq);                /* read == 0: todos os clientes fecharam REQ */
        close(fdresp);
    }
}
