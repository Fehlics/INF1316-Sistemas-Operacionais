#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Função para tratar o sinal de erro de operação aritmética (SIGFPE)
void trataSIGFPE(int sinal) {
    printf("\n[SINAL CAPTURADO] Erro Arithmetic Exception (SIGFPE) detectado!\n");
    printf("Motivo: Tentativa de divisao por zero.\n");
    // Encerra o programa para evitar loop infinito da instrucao invalida
    exit(1);
}

int main() {
    int a, b;

    // Registra a função tratadora para o sinal SIGFPE
    signal(SIGFPE, trataSIGFPE);

    printf("Digite o primeiro numero inteiro: ");
    if (scanf("%d", &a) != 1) return 1;

    printf("Digite o segundo numero inteiro: ");
    if (scanf("%d", &b) != 1) return 1;

    printf("\n--- Resultados das Operacoes ---\n");
    printf("Soma: %d + %d = %d\n", a, b, a + b);
    printf("Subtracao: %d - %d = %d\n", a, b, a - b);
    printf("Multiplicacao: %d * %d = %d\n", a, b, a * b);

    // A divisao por zero entre inteiros dispara o sinal SIGFPE
    printf("Divisao: %d / %d = %d\n", a, b, a / b);

    return 0;
}