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
            // TODO: LEITOR r: close(fd[1]); em loop, read(fd[0], buf, MSG) ate retornar != MSG;
            //       para cada msg imprima "    leitor r consumiu: buf" (fflush) e usleep(1000000).
            //       No fim: close(fd[0]); exit(0).
            // >>> escreva seu codigo aqui <<<
        }
    }

    close(fd[0]);
    // TODO: ESCRITOR (pai): para i = 1..10: snprintf(buf, MSG, "msg-%02d", i); write(fd[1], buf, MSG);
    //       imprima "escritor produziu: buf" e usleep(500000). Depois close(fd[1]) e wait pelos 2 filhos.
    // >>> escreva seu codigo aqui <<<
    return 0;
}
