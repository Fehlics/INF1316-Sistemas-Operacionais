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

    // TODO: 1) Pare os 3 filhos (SIGSTOP) para que so rode quem o escalonador escolher.
    // >>> escreva seu codigo aqui <<<

    // TODO: 2) Repita RODADAS vezes; em cada rodada, para cada processo i:
    //       imprima "[escalonador] P%d roda por %u s"; kill(p[i], SIGCONT);
    //       sleep(quantum[i]); kill(p[i], SIGSTOP);
    // >>> escreva seu codigo aqui <<<

    // TODO: 3) Mate (SIGKILL) e recolha (waitpid) os 3 filhos.
    // >>> escreva seu codigo aqui <<<
    return 0;
}
