#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main() {
    pid_t p1, p2, p3;

    // Criacao dos tres processos filhos
    if ((p1 = fork()) == 0) {
        execl("./p1", "p1", NULL);
        exit(1);
    }
    if ((p2 = fork()) == 0) {
        execl("./p2", "p2", NULL);
        exit(1);
    }
    if ((p3 = fork()) == 0) {
        execl("./p3", "p3", NULL);
        exit(1);
    }

    // Parada inicial de todos os processos
    kill(p1, SIGSTOP);
    kill(p2, SIGSTOP);
    kill(p3, SIGSTOP);

    printf("Simulador de Escalonamento Round-Robin iniciado...\n");

    // Loop do escalonador
    while (1) {
        // Fatia de tempo de P1: 1 segundo
        printf("\n---> [ESCALONADOR] Ativando P1 por 1 segundo\n");
        kill(p1, SIGCONT);
        sleep(1);
        kill(p1, SIGSTOP);

        // Fatia de tempo de P2: 2 segundos
        printf("\n---> [ESCALONADOR] Ativando P2 por 2 segundos\n");
        kill(p2, SIGCONT);
        sleep(2);
        kill(p2, SIGSTOP);

        // Fatia de tempo de P3: 2 segundos
        printf("\n---> [ESCALONADOR] Ativando P3 por 2 segundos\n");
        kill(p3, SIGCONT);
        sleep(2);
        kill(p3, SIGSTOP);
    }

    return 0;
}