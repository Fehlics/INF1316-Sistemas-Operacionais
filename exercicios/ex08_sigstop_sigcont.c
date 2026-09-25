/* ex08_sigstop_sigcont.c  --  Lab 3 (exercicio 4)
 * Dois filhos em loop infinito. O pai alterna quem roda com SIGSTOP/SIGCONT
 * (1 segundo cada). Depois de 10 trocas, o pai mata os dois.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static void filho_loop(char nome)
{
    for (;;) {
        printf("[filho %c] rodando (pid %d)\n", nome, getpid());
        fflush(stdout);
        usleep(300000);
    }
}

int main(void)
{
    pid_t f[2];
    for (int i = 0; i < 2; i++) {
        f[i] = fork();
        if (f[i] < 0) { perror("fork"); exit(1); }
        if (f[i] == 0) filho_loop('A' + i);
    }

    // TODO: Comece parando o filho B (kill(f[1], SIGSTOP)). Depois, 10 vezes:
    //       sleep(1); pare o filho atual (SIGSTOP); troque 'atual' (0<->1);
    //       acorde o novo atual (SIGCONT); imprima "troca N: agora roda o filho X".
    // >>> escreva seu codigo aqui <<<

    // TODO: Mate os dois filhos com SIGKILL e recolha-os com waitpid (evita zumbis).
    // >>> escreva seu codigo aqui <<<
    puts("[pai] fim.");
    return 0;
}
