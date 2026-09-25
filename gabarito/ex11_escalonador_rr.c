/* ex11_escalonador_rr.c  --  Lab 3 (exercicio 7, desafio)
 * Simula Round-Robin: P1 tem fatia de 1 s; P2 e P3, 2 s. Usa SIGSTOP/SIGCONT.
 * Compile o helper:  gcc -o io_loop io_loop.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 3
#define RODADAS 3

int main(void)
{
    const unsigned quantum[N] = {1, 2, 2};
    pid_t p[N];

    for (int i = 0; i < N; i++) {
        p[i] = fork();
        if (p[i] < 0) { perror("fork"); exit(1); }
        if (p[i] == 0) {
            char nome[8];
            snprintf(nome, sizeof nome, "P%d", i + 1);
            execl("./io_loop", "io_loop", nome, (char *)NULL);
            perror("execl");
            _exit(127);
        }
    }

    /* [resolvido] 1) Pare os 3 filhos (SIGSTOP) para que so rode quem o escalonador escolher. */
    for (int i = 0; i < N; i++) kill(p[i], SIGSTOP);

    /* [resolvido] 2) Repita RODADAS vezes; em cada rodada, para cada processo i: */
    for (int r = 0; r < RODADAS; r++)
        for (int i = 0; i < N; i++) {
            printf("[escalonador] P%d roda por %u s\n", i + 1, quantum[i]);
            fflush(stdout);
            kill(p[i], SIGCONT);
            sleep(quantum[i]);
            kill(p[i], SIGSTOP);
        }

    /* [resolvido] 3) Mate (SIGKILL) e recolha (waitpid) os 3 filhos. */
    for (int i = 0; i < N; i++) { kill(p[i], SIGKILL); waitpid(p[i], NULL, 0); }
    return 0;
}
