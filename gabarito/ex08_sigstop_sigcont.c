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

    /* [resolvido] Comece parando o filho B (kill(f[1], SIGSTOP)). Depois, 10 vezes: */
    kill(f[1], SIGSTOP);
    int atual = 0;
    for (int troca = 1; troca <= 10; troca++) {
        sleep(1);
        kill(f[atual], SIGSTOP);
        atual = 1 - atual;
        kill(f[atual], SIGCONT);
        printf("[pai] troca %d: agora roda o filho %c\n", troca, 'A' + atual);
    }

    /* [resolvido] Mate os dois filhos com SIGKILL e recolha-os com waitpid (evita zumbis). */
    kill(f[0], SIGKILL);
    kill(f[1], SIGKILL);
    waitpid(f[0], NULL, 0);
    waitpid(f[1], NULL, 0);
    puts("[pai] fim.");
    return 0;
}
