/* ex03_shm_contador.c  --  Lab 2 (exemplo do slide, refeito com checagem de erros)
 * Pai e filho compartilham UM int. Filho soma 5, pai (depois do wait) soma 10.
 * Saida esperada:  Processo filho = 8757  /  Processo pai = 8767
 * Depois de rodar:  ipcs -m   (nao deve sobrar segmento seu)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(void)
{
    int seg;
    int *p;

    /* [resolvido] 1) shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR) */
    seg = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (seg < 0) { perror("shmget"); exit(1); }

    /* [resolvido] 2) p = shmat(seg, NULL, 0). ATENCAO: a falha e (void *)-1, nao NULL. */
    p = shmat(seg, NULL, 0);
    if (p == (void *)-1) { perror("shmat"); exit(1); }

    *p = 8752;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        /* [resolvido] 3) FILHO: *p += 5; imprima "Processo filho = %d"; faca shmdt(p); exit(0). */
        *p += 5;
        printf("Processo filho = %d\n", *p);
        shmdt(p);
        exit(0);
    }

    waitpid(pid, NULL, 0);   /* <-- unica "sincronizacao" deste exemplo */
    *p += 10;
    printf("Processo pai = %d\n", *p);

    /* [resolvido] 4) PAI: shmdt(p) e depois shmctl(seg, IPC_RMID, NULL). */
    shmdt(p);
    shmctl(seg, IPC_RMID, NULL);
    return 0;
}
