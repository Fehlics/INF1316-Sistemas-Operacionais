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

    // TODO: 1) Crie o pipe: pipe(fd). fd[0] = leitura, fd[1] = escrita. Trate o erro.
    // >>> escreva seu codigo aqui <<<

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        // TODO: 2) FILHO: feche fd[0] (nao vai ler). Para cada msg em msgs[]: write(fd[1], msg,
        //       strlen(msg)+1), imprima "[filho] escreveu: ...", sleep(1). Ao fim: close(fd[1]); exit(0).
        // >>> escreva seu codigo aqui <<<
    }

    // TODO: 3) PAI: feche fd[1] (SEM isso o read nunca ve EOF!). Leia em loop com
    //       read(fd[0], buf, sizeof buf) ate retornar 0 e imprima "[pai] li N bytes: ...".
    //       Feche fd[0] e faca waitpid.
    // >>> escreva seu codigo aqui <<<
    puts("[pai] EOF: o filho fechou o pipe.");
    return 0;
}
