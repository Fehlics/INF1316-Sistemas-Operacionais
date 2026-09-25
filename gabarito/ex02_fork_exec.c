/* ex02_fork_exec.c  --  Lab 1 (exercicio 4)
 * Objetivo: fork + exec. Compile tambem o helper:  gcc -o alo alo.c
 * O 1o filho executa ./alo (com execl); o 2o filho executa o echo (com execvp).
 * O pai espera cada um e mostra o status de saida.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int status;

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }

        if (pid == 0) {
            if (i == 0) {
                /* [resolvido] 1o filho: execl("./alo", "alo", (char *)NULL); */
                execl("./alo", "alo", (char *)NULL);
            } else {
                /* [resolvido] 2o filho: monte char *args[] = {"echo", "alo", "do", "echo", NULL} */
                char *args[] = {"echo", "alo", "do", "echo", NULL};
                execvp("echo", args);
            }
            /* Se chegou aqui, o exec FALHOU (em caso de sucesso ele nunca retorna) */
            perror("exec");
            _exit(127);
        }

        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            printf("[pai] filho %d saiu com status %d\n", pid, WEXITSTATUS(status));
    }
    return 0;
}
