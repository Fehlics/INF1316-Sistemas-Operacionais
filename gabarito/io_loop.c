/* io_loop.c -- helper pronto: processo "I/O bound" que nunca termina */
#include <stdio.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    const char *nome = argc > 1 ? argv[1] : "io";
    for (;;) {
        printf("[%s] pid=%d fazendo I/O...\n", nome, getpid());
        fflush(stdout);
        usleep(200000);
    }
}
