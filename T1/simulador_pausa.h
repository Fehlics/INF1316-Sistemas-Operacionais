#ifndef SIMULADOR_PAUSA_H
#define SIMULADOR_PAUSA_H

#include "trabalho.h"
#include <signal.h>

int pausar_sistema(int controle, int fd_estado, pid_t controlador,
                  pid_t aplicativos[], int *restantes,
                  volatile sig_atomic_t *encerrar);
int retomar_sistema(int controle, int fd_estado, pid_t controlador);

#endif
