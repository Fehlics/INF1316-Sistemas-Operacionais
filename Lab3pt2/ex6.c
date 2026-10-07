#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

time_t tempo_inicio = 0;
int em_chamada = 0;

// Tratador para o inicio da chamada (SIGUSR1)
void iniciaChamada(int sinal) {
    tempo_inicio = time(NULL);
    em_chamada = 1;
    printf("\n[SIGUSR1] Chamada telefonica iniciada.\n");
}

// Tratador para o termino da chamada (SIGUSR2)
void finalizaChamada(int sinal) {
    if (!em_chamada) {
        printf("\n[SIGUSR2] Nenhuma chamada em andamento para finalizar.\n");
        return;
    }

    time_t tempo_fim = time(NULL);
    long duracao = (long)difftime(tempo_fim, tempo_inicio);
    double custo = 0.0;

    if (duracao <= 60) {
        // Ate 1 minuto (60 segundos): R$ 0.02 por segundo
        custo = duracao * 0.02;
    } else {
        // Primeiro minuto: 60 * 0.02 = R$ 1.20
        // A partir do 2º minuto: R$ 0.01 por segundo excedente
        custo = (60 * 0.02) + ((duracao - 60) * 0.01);
    }

    printf("\n[SIGUSR2] Chamada finalizada.\n");
    printf("Duracao: %ld segundos\n", duracao);
    printf("Custo total: R$ %.2f\n", custo);

    em_chamada = 0;
}

int main() {
    // Configura os manipuladores para SIGUSR1 e SIGUSR2
    signal(SIGUSR1, iniciaChamada);
    signal(SIGUSR2, finalizaChamada);

    int pid = getpid();
    printf("Programa de tarifacao iniciado.\n");
    printf("PID do processo: %d\n", pid);
    printf("Comando para iniciar: kill -s SIGUSR1 %d\n", pid);
    printf("Comando para finalizar: kill -s SIGUSR2 %d\n", pid);

    // Mantem o programa em execução aguardando sinais
    while (1) {
        pause();
    }

    return 0;
}