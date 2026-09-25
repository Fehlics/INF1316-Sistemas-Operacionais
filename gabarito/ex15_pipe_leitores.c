/* ex15_pipe_leitores.c  --  Lab 4 (exercicio 4): 1 escritor, 2 leitores, MESMO pipe.
 * Escritor: 1 mensagem a cada 0,5 s (10 no total). Leitores: 1 mensagem a cada 1 s.
 * Observe: cada mensagem e consumida por UM leitor so (o dado e removido ao ser lido).
 * Mensagens de tamanho fixo (MSG bytes) para nao "misturar" o stream.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MSG 16

int main(void)
{
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    for (int r = 1; r <= 2; r++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            /* [resolvido] LEITOR r: close(fd[1]); em loop, read(fd[0], buf, MSG) ate retornar != MSG; */
            char buf[MSG];
            close(fd[1]);
            while (read(fd[0], buf, MSG) == MSG) {
                printf("    leitor %d consumiu: %s\n", r, buf);
                fflush(stdout);
                usleep(1000000);
            }
            close(fd[0]);
            exit(0);
        }
    }

    close(fd[0]);
    /* [resolvido] ESCRITOR (pai): para i = 1..10: snprintf(buf, MSG, "msg-%02d", i); write(fd[1], buf, MSG); */
    for (int i = 1; i <= 10; i++) {
        char buf[MSG];
        snprintf(buf, MSG, "msg-%02d", i);
        write(fd[1], buf, MSG);
        printf("escritor produziu: %s\n", buf);
        fflush(stdout);
        usleep(500000);
    }
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    return 0;
}
