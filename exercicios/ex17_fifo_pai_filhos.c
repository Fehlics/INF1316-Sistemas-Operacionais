/* ex17_fifo_pai_filhos.c  --  Lab 6 (exercicio 2)
 * O pai cria a FIFO e 2 filhos; cada filho escreve UMA string; o pai da waitpid nos dois
 * e SO ENTAO le as strings.
 * ARMADILHA: open(O_WRONLY) bloqueia ate existir um leitor. Se o pai fizesse waitpid
 * ANTES de abrir a FIFO para leitura -> deadlock. Solucao: o pai abre a FIFO
 * para leitura com O_NONBLOCK ANTES de criar os filhos.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FIFO "fifo_filhos"

int main(void)
{
    if (mkfifo(FIFO, S_IRUSR | S_IWUSR) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }

    // TODO: 1) fdr = open(FIFO, O_RDONLY | O_NONBLOCK)  (retorna na hora, mesmo sem escritor)
    // >>> escreva seu codigo aqui <<<

    pid_t pid[2];
    for (int i = 0; i < 2; i++) {
        pid[i] = fork();
        if (pid[i] < 0) { perror("fork"); return 1; }
        if (pid[i] == 0) {
            // TODO: 2) FILHO: close(fdr); fdw = open(FIFO, O_WRONLY); monte "ola, sou o filho i (pid X)"
            //       com snprintf; write(fdw, msg, strlen(msg)+1); close(fdw); _exit(0).
            // >>> escreva seu codigo aqui <<<
        }
    }

    // TODO: 3) PAI: waitpid nos dois filhos.
    // >>> escreva seu codigo aqui <<<

    // TODO: 4) PAI: leia da FIFO (read em loop ate retornar 0) e imprima cada string
    //       (as strings vem separadas por '\0' dentro do buffer!). Depois close(fdr) e unlink(FIFO).
    // >>> escreva seu codigo aqui <<<
    return 0;
}
