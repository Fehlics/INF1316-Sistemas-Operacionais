/* ex14_pipeline.c  --  Lab 4 (exercicio 3): o que a shell faz em   ls -l | wc -l
 * (troque para "ps" | "wc" se quiser igualar ao slide)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork"); exit(1); }
    if (p1 == 0) {
        // TODO: FILHO 1 (produtor): dup2(fd[1], STDOUT_FILENO); close(fd[0]); close(fd[1]);
        //       execlp("ls", "ls", "-l", (char *)NULL); perror + _exit(127) se falhar.
        // >>> escreva seu codigo aqui <<<
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork"); exit(1); }
    if (p2 == 0) {
        // TODO: FILHO 2 (consumidor): dup2(fd[0], STDIN_FILENO); feche fd[0] e fd[1];
        //       execlp("wc", "wc", "-l", (char *)NULL).
        // >>> escreva seu codigo aqui <<<
    }

    // TODO: PAI: feche fd[0] e fd[1] (se esquecer, o wc NUNCA termina: por que?)
    //       e espere os dois filhos.
    // >>> escreva seu codigo aqui <<<
    return 0;
}
