/* dorme.c -- helper pronto: ./dorme N  (dorme N segundos) */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    int s = argc > 1 ? atoi(argv[1]) : 5;
    printf("indo dormir...\n"); fflush(stdout);
    sleep(s);
    printf("Acordei!\n");
    return 0;
}
