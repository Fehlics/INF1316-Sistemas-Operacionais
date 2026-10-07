#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO "minha_fifo_ex1"

int main() {
    // Cria a FIFO com permissoes de leitura e escrita
    mkfifo(FIFO, 0666);
    
    printf("Aguardando entrada (abra o escritor em outro terminal)...\n");
    // A chamada a open bloqueia ate o escritor abrir a FIFO
    int fd = open(FIFO, O_RDONLY);
    
    char buffer[256];
    int bytes_lidos;
    
    // Fica em loop a ler da FIFO e a escrever no ecra
    while ((bytes_lidos = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_lidos);
    }
    
    close(fd);
    unlink(FIFO); // Remove a FIFO no final
    return 0;
}