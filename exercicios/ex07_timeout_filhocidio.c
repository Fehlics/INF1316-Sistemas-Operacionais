/* ex07_timeout_filhocidio.c  --  Lab 3 (exercicio 3: "filhocidio")
 * Uso: ./ex07_timeout_filhocidio SEGUNDOS programa [args...]
 *   ./ex07... 10 ./dorme 5    -> termina a tempo
 *   ./ex07... 3  ./dorme 15   -> o pai mata o filho por estouro de tempo
 * Compile o helper:  gcc -o dorme dorme.c
 * OBS: esta versao usa SIGALRM. A do slide (filhocidio.c) usa sleep() no pai + handler de
 * SIGCHLD. Depois de fazer esta, leia a do slide e compare os dois desenhos (veja o guia).
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static pid_t filho;
static volatile sig_atomic_t estourou = 0;

static void trata_alarme(int sinal)
{
    (void)sinal;
    // TODO: marque estourou = 1 e mate o filho: kill(filho, SIGKILL)
    // >>> escreva seu codigo aqui <<<
}

int main(int argc, char *argv[])
{
    if (argc < 3) { fprintf(stderr, "uso: %s segundos prog [args...]\n", argv[0]); return 1; }
    int limite = atoi(argv[1]);
    int status = 0;

    filho = fork();
    if (filho < 0) { perror("fork"); return 1; }

    if (filho == 0) {
        // TODO: FILHO: execvp(argv[2], &argv[2]); se retornar, perror + _exit(127).
        // >>> escreva seu codigo aqui <<<
    }

    // TODO: PAI: signal(SIGALRM, trata_alarme); alarm(limite);
    //       waitpid(filho, &status, 0) repetindo se errno == EINTR; depois alarm(0) (desarma).
    // >>> escreva seu codigo aqui <<<

    if (estourou)
        printf("Programa %s excedeu o limite de %d segundos!\n", argv[2], limite);
    else if (WIFEXITED(status))
        printf("Filho %d terminou dentro de %d segundos com estado %d.\n",
               filho, limite, WEXITSTATUS(status));
    return 0;
}
