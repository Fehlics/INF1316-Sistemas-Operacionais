#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO "minha_fifo_ex1"

int main() {
    // A chamada a open bloqueia ate o leitor abrir a FIFO
    int fd = open(FIFO, O_WRONLY);
    
    char buffer[256];
    int bytes_lidos;
    
    printf("Introduza a mensagem (Ctrl+D para terminar):\n");
    // Fica a ler do teclado (stdin) e a escrever na FIFO
    while ((bytes_lidos = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        write(fd, buffer, bytes_lidos);
    }
    
    close(fd);
    return 0;
}