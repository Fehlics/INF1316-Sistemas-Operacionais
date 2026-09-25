/* ex12_pipe_pai_filho.c  --  Lab 4 (exercicio 1)
 * O FILHO escreve 3 mensagens no pipe (1 por segundo); o PAI le e mostra.
 * Pergunta: o que muda se voce tirar o sleep(1) do filho? (pense: pipe = STREAM de bytes)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];

    /* [resolvido] 1) Crie o pipe: pipe(fd). fd[0] = leitura, fd[1] = escrita. Trate o erro. */
    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        /* [resolvido] 2) FILHO: feche fd[0] (nao vai ler). Para cada msg em msgs[]: write(fd[1], msg, */
        const char *msgs[] = {"primeira", "segunda", "terceira"};
        close(fd[0]);
        for (int i = 0; i < 3; i++) {
            write(fd[1], msgs[i], strlen(msgs[i]) + 1);
            printf("[filho] escreveu: %s\n", msgs[i]);
            fflush(stdout);
            sleep(1);
        }
        close(fd[1]);
        exit(0);
    }

    /* [resolvido] 3) PAI: feche fd[1] (SEM isso o read nunca ve EOF!). Leia em loop com */
    char buf[64];
    ssize_t n;
    close(fd[1]);
    while ((n = read(fd[0], buf, sizeof buf - 1)) > 0) {
        buf[n] = '\0';
        printf("[pai] li %zd bytes: %s\n", n, buf);
    }
    close(fd[0]);
    waitpid(pid, NULL, 0);
    puts("[pai] EOF: o filho fechou o pipe.");
    return 0;
}
