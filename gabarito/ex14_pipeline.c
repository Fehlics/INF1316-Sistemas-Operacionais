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
        /* [resolvido] FILHO 1 (produtor): dup2(fd[1], STDOUT_FILENO); close(fd[0]); close(fd[1]); */
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp ls");
        _exit(127);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork"); exit(1); }
    if (p2 == 0) {
        /* [resolvido] FILHO 2 (consumidor): dup2(fd[0], STDIN_FILENO); feche fd[0] e fd[1]; */
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("execlp wc");
        _exit(127);
    }

    /* [resolvido] PAI: feche fd[0] e fd[1] (se esquecer, o wc NUNCA termina: por que?) */
    close(fd[0]);
    close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
    return 0;
}
