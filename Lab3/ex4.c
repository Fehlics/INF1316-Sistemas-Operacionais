#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;

    // Cria Filho 1
    if ((pid1 = fork()) == 0) {
        while (1) {
            printf("[Filho 1] Executando...\n");
            sleep(1);
        }
    }

    // Cria Filho 2
    if ((pid2 = fork()) == 0) {
        while (1) {
            printf("[Filho 2] Executando...\n");
            sleep(1);
        }
    }

    kill(pid1, SIGSTOP);
    kill(pid2, SIGSTOP);
    sleep(1);
    
    printf("Iniciando as 10 trocas de contexto...\n");

    for (int i = 0; i < 5; i++) {
        // Ativa Filho 1 e pausa Filho 2
        printf("\n--- Troca de Contexto %d ---\n", (i * 2) + 1);
        kill(pid2, SIGSTOP);
        kill(pid1, SIGCONT);
        sleep(2);

        // Pausa Filho 1 e ativa Filho 2
        printf("\n--- Troca de Contexto %d ---\n", (i * 2) + 2);
        kill(pid1, SIGSTOP);
        kill(pid2, SIGCONT);
        sleep(2);
    }

    // Pai mata os filhos após 10 trocas
    printf("\nLimite de trocas atingido. Encerrando os processos filhos...\n");
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);

    wait(NULL);
    wait(NULL);

    printf("Processo pai finalizado com sucesso.\n");
    return 0;
}