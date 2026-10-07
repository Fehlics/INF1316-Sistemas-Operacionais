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
        close(fd[1]);
        int dado;
        while (read(fd[0], &dado, sizeof(int)) > 0) {
            printf("Leitor 1 consumiu o dado: %d\n", dado);
            sleep(2);
        }
        close(fd[0]);
        exit(0);
    }

    if (fork() == 0) {
        close(fd[1]);
        int dado;
        while (read(fd[0], &dado, sizeof(int)) > 0) {
            printf("Leitor 2 consumiu o dado: %d\n", dado);
            sleep(2);
        }
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);
    for (int i = 1; i <= 6; i++) {
        printf("Escritor produziu o dado: %d\n", i);
        write(fd[1], &i, sizeof(int));
        sleep(1);
    }
    
    close(fd[1]); 
    wait(NULL);
    wait(NULL);
    printf("Todos os processos finalizaram.\n");

    return 0;
}