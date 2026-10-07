#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd_in = open("entrada.txt", O_RDONLY);
    if (fd_in < 0) {
        perror("Erro ao abrir entrada.txt");
        exit(1);
    }

    int fd_out = open("saida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out < 0) {
        perror("Erro ao criar saida.txt");
        exit(1);
    }

    dup2(fd_in, 0); 
    dup2(fd_out, 1); 

    close(fd_in);
    close(fd_out);

    char c;
    while (read(0, &c, 1) > 0) {
        write(1, &c, 1);
    }

    return 0;
}