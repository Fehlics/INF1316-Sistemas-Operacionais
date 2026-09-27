#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* As mensagens pequenas sao enviadas em uma unica escrita na pipe. */
int enviar_dados(int fd, const void *dados, size_t tamanho);
int receber_dados(int fd, void *dados, size_t tamanho);
int intervalo_teste(const char *variavel, int padrao, int minimo);

#endif
