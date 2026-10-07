#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

const char *FIFO_REQ = "fifo_req";
const char *FIFO_RESP = "fifo_resp";

int main(void) {
    char linha[256];
    char resposta[256];
    int fd_req, fd_resp, n;

    printf("Cliente: digite uma palavra ou frase (ou 'sair' para terminar)\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(linha, sizeof(linha), stdin) == NULL)
            break; // Ctrl+D

        if (strncmp(linha, "sair", 4) == 0)
            break;

        // ---- 1) Envia a linha para o servidor ----
        fd_req = open(FIFO_REQ, O_WRONLY);
        if (fd_req < 0) {
            perror("open fifo_req (o servidor esta rodando?)");
            continue;
        }
        write(fd_req, linha, strlen(linha));
        close(fd_req);

        // ---- 2) Espera a resposta do servidor ----
        fd_resp = open(FIFO_RESP, O_RDONLY);
        if (fd_resp < 0) {
            perror("open fifo_resp");
            continue;
        }
        n = read(fd_resp, resposta, sizeof(resposta) - 1);
        close(fd_resp);

        // ---- 3) Exibe a resposta ----
        if (n > 0) {
            resposta[n] = '\0';
            printf("Resposta do servidor: %s", resposta);
        }
    }

    return 0;
}