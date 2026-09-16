#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    
    if (pipe(fd) < 0) {
        perror("Erro ao criar pipe");
        exit(1);
    }

    if (fork() == 0) {
        close(fd[0]);
        dup2(fd[1], 1); 
        close(fd[1]);
        
        execlp("ps", "ps", NULL);
        perror("Erro no exec ps");
        exit(1);
    }

    if (fork() == 0) {
        close(fd[1]);
        dup2(fd[0], 0); 
        close(fd[0]);
        
        execlp("wc", "wc", NULL);
        perror("Erro no exec wc");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);

    return 0;
}