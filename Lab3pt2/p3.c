#include <stdio.h>
#include <unistd.h>

int main() {
    while (1) {
        printf("[Processo P3] Escrevendo mensagem na tela...\n");
        usleep(300000);
    }
    return 0;
}