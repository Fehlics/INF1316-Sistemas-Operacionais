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

    /* [resolvido] 1) fdr = open(FIFO, O_RDONLY | O_NONBLOCK)  (retorna na hora, mesmo sem escritor) */
    int fdr = open(FIFO, O_RDONLY | O_NONBLOCK);
    if (fdr < 0) { perror("open leitura"); return 1; }

    pid_t pid[2];
    for (int i = 0; i < 2; i++) {
        pid[i] = fork();
        if (pid[i] < 0) { perror("fork"); return 1; }
        if (pid[i] == 0) {
            /* [resolvido] 2) FILHO: close(fdr); fdw = open(FIFO, O_WRONLY); monte "ola, sou o filho i (pid X)" */
            char msg[64];
            close(fdr);
            int fdw = open(FIFO, O_WRONLY);
            if (fdw < 0) { perror("open escrita"); _exit(1); }
            snprintf(msg, sizeof msg, "ola, sou o filho %d (pid %d)", i + 1, getpid());
            write(fdw, msg, strlen(msg) + 1);
            close(fdw);
            _exit(0);
        }
    }

    /* [resolvido] 3) PAI: waitpid nos dois filhos. */
    waitpid(pid[0], NULL, 0);
    waitpid(pid[1], NULL, 0);

    /* [resolvido] 4) PAI: leia da FIFO (read em loop ate retornar 0) e imprima cada string */
    char buf[256];
    ssize_t n;
    while ((n = read(fdr, buf, sizeof buf)) > 0) {
        for (ssize_t k = 0; k < n; ) {
            printf("[pai] recebi: %s\n", buf + k);
            k += (ssize_t)strlen(buf + k) + 1;
        }
    }
    close(fdr);
    unlink(FIFO);
    return 0;
}
