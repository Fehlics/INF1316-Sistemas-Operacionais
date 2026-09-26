#include <stdio.h>
#include "testes.h"

int main(void) {

    printf("Iniciando testes...\n\n");

    testar_processos();
    testar_escalonamento();
    testar_syscalls();

    printf("\nTestes finalizados.\n");

    return 0;
}