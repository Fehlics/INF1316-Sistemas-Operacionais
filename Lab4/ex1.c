#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pid_t pid;
    char mensagemEscrita[] = "Hello World!";
    char bufferLeitura[100];

    if (pipe(fd) < 0) {
        perror("Erro ao criar o pipe");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        perror("Erro no fork");
        exit(1);
    }

    if (pid == 0) {
        close(fd[0]);
        
        printf("[FILHO] Escrevendo no pipe: '%s'\n", mensagemEscrita);
        write(fd[1], mensagemEscrita, strlen(mensagemEscrita) + 1);
        
        close(fd[1]);
        exit(0);
    } else {
        close(fd[1]);
        
        wait(NULL);
        
        read(fd[0], bufferLeitura, sizeof(bufferLeitura));
        printf("[PAI] Mensagem lida do pipe: '%s'\n", bufferLeitura);
        
        close(fd[0]);
    }

    return 0;
}