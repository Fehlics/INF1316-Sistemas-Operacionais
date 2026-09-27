#include "util.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int enviar_dados(int fd, const void *dados, size_t tamanho) {
    ssize_t n;
    do { n = write(fd, dados, tamanho); }
    while (n < 0 && errno == EINTR);
    return n == (ssize_t)tamanho;
}

int receber_dados(int fd, void *dados, size_t tamanho) {
    size_t total = 0;
    while (total < tamanho) {
        ssize_t n = read(fd, (char *)dados + total, tamanho - total);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        total += (size_t)n;
    }
    return 1;
}

int intervalo_teste(const char *variavel, int padrao, int minimo) {
    const char *texto = getenv(variavel);
    if (!texto) return padrao;
    char *fim;
    long valor = strtol(texto, &fim, 10);
    return *fim == '\0' && valor >= minimo && valor <= 500000 ?
           (int)valor : padrao;
}
