/*
 * Exercicio 3 - Servidor (roda em background)
 * (VERSAO CORRIGIDA: limpa as FIFOs ao terminar)
 *
 * Usa DUAS FIFOs para comunicacao bidirecional:
 *   fifo_req  -> clientes ESCREVEM, servidor LE
 *   fifo_resp -> servidor ESCREVE, clientes LEEM
 *
 * Para limpar as FIFOs ao terminar: use Ctrl+C ou kill <PID>.
 * O signal handler vai remover as FIFOs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

const char *FIFO_REQ = "fifo_req";
const char *FIFO_RESP = "fifo_resp";

// Handler para sinais (SIGINT = Ctrl+C, SIGTERM = kill)
// Remove as duas FIFOs antes de terminar
void cleanup(int sig) {
    printf("\nServidor terminando...\n");
    unlink(FIFO_REQ);
    unlink(FIFO_RESP);
    exit(0);
}

int main(void) {
    int fd_req, fd_resp;
    char linha[256];
    int i, n;

    // Registra o handler para remover FIFOs ao receber sinais
    signal(SIGINT, cleanup);   // Ctrl+C
    signal(SIGTERM, cleanup);  // kill

    // Cria as duas FIFOs, se ainda nao existirem
    if (access(FIFO_REQ, F_OK) == -1)
        mkfifo(FIFO_REQ, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (access(FIFO_RESP, F_OK) == -1)
        mkfifo(FIFO_RESP, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    fd_req = open(FIFO_REQ, O_RDONLY | O_NONBLOCK);
    if (fd_req < 0) {
        perror("open fifo_req");
        exit(1);
    }

    // Loop principal do servidor: fica sempre rodando
    while (1) {
        n = read(fd_req, linha, sizeof(linha) - 1);

        if (n > 0) {
            linha[n] = '\0';

            // Converte cada caractere da linha para maiuscula
            for (i = 0; i < n; i++)
                linha[i] = toupper((unsigned char) linha[i]);

            // Abre a fifo_resp so na hora de responder
            fd_resp = open(FIFO_RESP, O_WRONLY);
            if (fd_resp >= 0) {
                write(fd_resp, linha, n);
                close(fd_resp);
            }
        } else {
            // Nao havia dados agora: espera um pouco
            usleep(100000);
        }
    }

    close(fd_req);
    return 0;
}