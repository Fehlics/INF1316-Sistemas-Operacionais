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

    // TODO: Crie as duas FIFOs com mkfifo (aceite EEXIST).
    // >>> escreva seu codigo aqui <<<

    for (;;) {                       /* uma iteracao = uma "sessao" de cliente */
        int fdreq, fdresp;
        // TODO: Abra REQ com O_RDONLY e DEPOIS RESP com O_WRONLY (nessa ordem! o cliente abre
        //       na mesma ordem: REQ escrita, RESP leitura -> sem deadlock). Ambos bloqueiam ate o par abrir.
        // >>> escreva seu codigo aqui <<<

        char buf[256];
        ssize_t n;
        // TODO: Enquanto read(fdreq, buf, sizeof buf) > 0: converta cada byte com toupper() e
        //       envie de volta com write(fdresp, buf, n).
        // >>> escreva seu codigo aqui <<<

        close(fdreq);                /* read == 0: todos os clientes fecharam REQ */
        close(fdresp);
    }
}
