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

    // TODO: 1) shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)
    //       Guarde em 'seg'. Se < 0: perror + exit.
    // >>> escreva seu codigo aqui <<<

    // TODO: 2) p = shmat(seg, NULL, 0). ATENCAO: a falha e (void *)-1, nao NULL.
    // >>> escreva seu codigo aqui <<<

    *p = 8752;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        // TODO: 3) FILHO: *p += 5; imprima "Processo filho = %d"; faca shmdt(p); exit(0).
        //       (o filho NAO deve fazer IPC_RMID: quem remove e so um processo, o pai)
        // >>> escreva seu codigo aqui <<<
    }

    waitpid(pid, NULL, 0);   /* <-- unica "sincronizacao" deste exemplo */
    *p += 10;
    printf("Processo pai = %d\n", *p);

    // TODO: 4) PAI: shmdt(p) e depois shmctl(seg, IPC_RMID, NULL).
    // >>> escreva seu codigo aqui <<<
    return 0;
}
