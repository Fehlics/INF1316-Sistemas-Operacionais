/* ex01_fork_wait.c  --  Lab 1 (exercicios 1, 2 e 3)
 * Objetivo: fork(), getpid/getppid, waitpid, exit status e "memoria separada".
 * Saida esperada (ordem pode variar so entre linhas do pai/filho antes do wait):
 *   [pai]   pid=... x antes do fork = 1
 *   [filho] pid=... ppid=... / [filho] x = 5
 *   [pai]   filho ... terminou com status 3
 *   [pai]   x depois do waitpid = 1      <-- por que 1 e nao 5?
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int x = 1;
    int status = 0;
    pid_t pid;

    printf("[pai]   pid=%d  x antes do fork = %d\n", getpid(), x);
    fflush(stdout);   /* sem isto, com a saida redirecionada (> arquivo / | ) o texto
                         ainda no buffer seria DUPLICADO no filho pelo fork()! */

    /* [resolvido] 1) chame fork() e guarde o retorno em 'pid'. */
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        /* [resolvido] 2) FILHO: imprima getpid() e getppid(); mude x para 5; imprima x; */
        printf("[filho] pid=%d ppid=%d\n", getpid(), getppid());
        x = 5;
        printf("[filho] x = %d\n", x);
        exit(3);
    }

    /* [resolvido] 3) PAI: espere o filho com waitpid(pid, &status, 0). */
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        printf("[pai]   filho %d terminou com status %d\n", pid, WEXITSTATUS(status));

    printf("[pai]   x depois do waitpid = %d\n", x);
    return 0;
}
