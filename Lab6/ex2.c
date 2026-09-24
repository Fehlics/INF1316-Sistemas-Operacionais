#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

const char *NOME_FIFO = "fifo_ex2";

int main(void) {
    int pid1, pid2, status;
    int fifo_leitura;

    // ---- 1) Cria a FIFO ----
    if (access(NOME_FIFO, F_OK) == -1) {
        if (mkfifo(NOME_FIFO, S_IRUSR | S_IWUSR) != 0) {
            perror("mkfifo");
            exit(1);
        }
    }

    fifo_leitura = open(NOME_FIFO, O_RDONLY | O_NONBLOCK);
    if (fifo_leitura < 0) {
        perror("open (leitura)");
        exit(1);
    }

    // ---- 2) Cria o primeiro processo filho ----
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(1);
    } else if (pid1 == 0) {
        int f = open(NOME_FIFO, O_WRONLY);
        char msg[] = "Mensagem do filho 1\n";
        write(f, msg, strlen(msg));
        close(f);
        exit(0);
    }

    // ---- 3) Cria o segundo processo filho ----
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(1);
    } else if (pid2 == 0) {
        int f = open(NOME_FIFO, O_WRONLY);
        char msg[] = "Mensagem do filho 2\n";
        write(f, msg, strlen(msg));
        close(f);
        exit(0);
    }

    // ---- 4) Pai espera os dois filhos terminarem ----
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // ---- 5) Pai le o que os filhos escreveram ----
    printf("Pai: lendo o que os filhos escreveram na FIFO:\n");
    char ch;
    while (read(fifo_leitura, &ch, sizeof(ch)) > 0)
        putchar(ch);

    close(fifo_leitura);

    // ---- 6) Remove a FIFO antes de terminar (NOVO) ----
    unlink(NOME_FIFO);

    return 0;
}